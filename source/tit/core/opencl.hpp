/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <initializer_list>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/enum_utils.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/type_utils.hpp"
#include "tit/core/utils.hpp"

// NOLINTBEGIN(*-reserved-identifier,cert-*)
using cl_platform_id = struct _cl_platform_id*;
using cl_device_id = struct _cl_device_id*;
using cl_context = struct _cl_context*;
using cl_command_queue = struct _cl_command_queue*;
using cl_mem = struct _cl_mem*;
using cl_program = struct _cl_program*;
using cl_kernel = struct _cl_kernel*;
// NOLINTEND(*-reserved-identifier,cert-*)

namespace tit::ocl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenCL platform.
class Platform final {
public:

  /// Enumerate all available platforms.
  static auto all() -> std::vector<Platform>;

  /// Get the default platform.
  static auto default_() -> Platform;

  /// Get the platform object.
  auto base() const noexcept -> cl_platform_id;

  /// Get the platform name.
  auto name() const -> std::string;

  /// Get overall platform information.
  auto info() const -> std::string;

private:

  explicit Platform(cl_platform_id platform) noexcept : platform_{platform} {}

  cl_platform_id platform_ = nullptr;

}; // class Platform

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Device type.
enum class DeviceTypes : uint8_t {
  none = 0,
  cpu = 1 << 0,
  gpu = 1 << 1,
  accelerator = 1 << 2,
  default_ = 1 << 3,
  all = cpu | gpu | accelerator | default_,
};

/// OpenCL device.
class Device final {
public:

  /// Enumerate all available devices.
  static auto all(const Platform& platform,
                  DeviceTypes types = DeviceTypes::all) -> std::vector<Device>;

  /// Get the default device.
  static auto default_(const Platform& platform) -> Device;

  /// Get the device object.
  auto base() const noexcept -> cl_device_id;

  /// Get the device name.
  auto name() const -> std::string;

private:

  explicit Device(cl_device_id device) noexcept : device_{device} {}

  struct Retainer_ final {
    void operator()(cl_device_id device) const;
  };
  struct Releaser_ final {
    void operator()(cl_device_id device) const;
  };

  Shared<cl_device_id, Retainer_, Releaser_> device_;

}; // class Device

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenCL context.
class Context final {
public:

  /// Construct a context.
  Context(const std::vector<Device>& devices);

  /// Get the context object.
  auto base() const noexcept -> cl_context;

private:

  struct Retainer_ final {
    void operator()(cl_context context) const;
  };
  struct Releaser_ final {
    void operator()(cl_context context) const;
  };

  Shared<cl_context, Retainer_, Releaser_> context_;

}; // class Context

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenCL command queue.
class CommandQueue final {
public:

  /// Construct a command queue.
  CommandQueue(Context& context, const Device& device);

  /// Get the command queue object.
  auto base() const noexcept -> cl_command_queue;

  /// Flush the command queue.
  void flush() const;

  /// Wait for the command queue to finish.
  void finish() const;

private:

  struct Retainer_ final {
    void operator()(cl_command_queue queue) const;
  };
  struct Releaser_ final {
    void operator()(cl_command_queue queue) const;
  };

  Shared<cl_command_queue, Retainer_, Releaser_> queue_;

}; // class CommandQueue

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenCL buffer access flags.
enum class BufferAccess : uint8_t {
  none = 0,
  host_read = 1 << 0,
  host_write = 1 << 1,
  host_read_write = host_read | host_write,
  device_read = 1 << 2,
  device_write = 1 << 3,
  device_read_write = device_read | device_write,
  all = host_read_write | device_read_write,
};

/// OpenCL buffer base.
class BaseMem {
public:

  /// Get the buffer object.
  auto base() const noexcept -> cl_mem;

  /// Get the buffer width.
  auto width() const -> size_t;

protected:

  BaseMem() = default;

  BaseMem(Context& context,
          BufferAccess access,
          size_t width,
          const byte_t* data = nullptr);

  void enqueue_read_(CommandQueue& queue, std::span<byte_t> data) const;

  void enqueue_write_(CommandQueue& queue, std::span<const byte_t> data);

private:

  struct Retainer_ final {
    void operator()(cl_mem mem) const;
  };
  struct Releaser_ final {
    void operator()(cl_mem mem) const;
  };

  Shared<cl_mem, Retainer_, Releaser_> mem_;

}; // class BaseMem

/// OpenCL buffer.
template<class Val>
  requires std::is_trivially_copyable_v<Val>
class Mem final : public BaseMem {
public:

  /// Construct a null buffer.
  Mem() = default;

  /// Construct a buffer of the given type.
  Mem(Context& context, BufferAccess access, size_t size)
      : BaseMem{context, access, size * sizeof(Val)} {}

  /// Construct a buffer with data.
  Mem(Context& context, BufferAccess access, std::span<const Val> data)
      : BaseMem{context,
                access,
                data.size() * sizeof(Val),
                safe_bit_ptr_cast<const byte_t*>(data.data())} {}

  /// Get the buffer size.
  auto size() const -> size_t {
    return this->width() / sizeof(Val);
  }

  /// Enqueue a command to read the buffer.
  void enqueue_read(CommandQueue& queue, std::span<Val> data) const {
    const std::span bytes{safe_bit_ptr_cast<byte_t*>(data.data()),
                          data.size() * sizeof(Val)};
    BaseMem::enqueue_read_(queue, bytes);
  }

  /// Enqueue a command to write the buffer.
  void enqueue_write(CommandQueue& queue, std::span<const Val> data) {
    const std::span bytes{safe_bit_ptr_cast<const byte_t*>(data.data()),
                          data.size() * sizeof(Val)};
    BaseMem::enqueue_write_(queue, bytes);
  }

}; // class Mem

template<class Val>
Mem(Context&, BufferAccess, std::span<Val>) -> Mem<std::remove_const_t<Val>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// OpenCL program.
class Program final {
public:

  explicit Program(Context& context, const Device& device, CStrView source);

  /// Get the program object.
  auto base() const noexcept -> cl_program;

private:

  struct Retainer_ final {
    void operator()(cl_program program) const;
  };
  struct Releaser_ final {
    void operator()(cl_program program) const;
  };

  Shared<cl_program, Retainer_, Releaser_> program_;

}; // class Program

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Simple kernel argument type.
template<class Val>
concept simple_kernel_arg = std::is_trivially_copyable_v<Val>;

/// OpenCL kernel.
/// @todo We might want a template for the kernel arguments.
class Kernel final {
public:

  /// Construct a kernel.
  Kernel(const Program& program, CStrView name);

  /// Get the kernel object.
  auto base() const noexcept -> cl_kernel;

  /// Get the kernel name.
  auto name() const -> std::string;

  /// Get the kernel argument count.
  auto num_args() const -> size_t;

  /// Set the kernel argument.
  /// @{
  void set_arg(size_t index, const simple_kernel_arg auto& val) {
    set_arg_(index, val);
  }
  void set_arg(size_t index, BaseMem& data) {
    set_arg_(index, data.base());
  }
  /// @}

  /// Set all kernel arguments.
  template<class... Args>
  void set_args(Args&&... args) {
    size_t index = 0;
    (set_arg(index++, std::forward<Args>(args)), ...);
  }

  /// Enqueue the kernel for execution.
  /// @{
  void enqueue_exec(CommandQueue& queue,
                    std::span<const size_t> global_work_offset,
                    std::span<const size_t> global_work_size,
                    std::span<const size_t> local_work_size = {}) const;
  void enqueue_exec(CommandQueue& queue,
                    std::initializer_list<size_t> global_work_offset,
                    std::initializer_list<size_t> global_work_size,
                    std::initializer_list<size_t> local_work_size = {}) const {
    /// @todo In C++26 there would be no need for this overload.
    enqueue_exec(queue,
                 std::span{global_work_offset},
                 std::span{global_work_size},
                 std::span{local_work_size});
  }
  /// @}

private:

  template<class Val>
    requires std::is_trivially_copyable_v<Val>
  void set_arg_(size_t index, const Val& val) {
    set_arg_bytes_(
        index, // NOLINTNEXTLINE(bugprone-sizeof-expression)
        std::span{safe_bit_ptr_cast<const byte_t*>(&val), sizeof(val)});
  }

  void set_arg_bytes_(size_t index, std::span<const byte_t> data);

  struct Retainer_ final {
    void operator()(cl_kernel kernel) const;
  };
  struct Releaser_ final {
    void operator()(cl_kernel kernel) const;
  };

  Shared<cl_kernel, Retainer_, Releaser_> kernel_;

}; // class Kernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ocl

template<>
inline constexpr auto tit::is_flags_enum_v<tit::ocl::DeviceTypes> = true;

template<>
inline constexpr auto tit::is_flags_enum_v<tit::ocl::BufferAccess> = true;
