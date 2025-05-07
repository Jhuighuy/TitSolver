/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <format>
#include <random>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#define CL_SILENCE_DEPRECATION
#define CL_TARGET_OPENCL_VERSION 120 // NOLINT(*-macro-to-enum)

#if __has_include(<CL/cl.h>)
#include <CL/cl.h>
#include <CL/cl_platform.h>
#elif __has_include(<OpenCL/cl.h>)
#include <OpenCL/cl.h>
#include <OpenCL/cl_platform.h>
#else
#error "OpenCL headers not found!"
#endif

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/opencl.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/utils.hpp"

namespace tit::ocl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto error_message(cl_int status) -> CStrView {
  return translate<CStrView>(status)
      .option(CL_SUCCESS, "CL_SUCCESS")
      .option(CL_DEVICE_NOT_FOUND, "CL_DEVICE_NOT_FOUND")
      .option(CL_DEVICE_NOT_AVAILABLE, "CL_DEVICE_NOT_AVAILABLE")
      .option(CL_COMPILER_NOT_AVAILABLE, "CL_COMPILER_NOT_AVAILABLE")
      .option(CL_MEM_OBJECT_ALLOCATION_FAILURE,
              "CL_MEM_OBJECT_ALLOCATION_FAILURE")
      .option(CL_OUT_OF_RESOURCES, "CL_OUT_OF_RESOURCES")
      .option(CL_OUT_OF_HOST_MEMORY, "CL_OUT_OF_HOST_MEMORY")
      .option(CL_PROFILING_INFO_NOT_AVAILABLE,
              "CL_PROFILING_INFO_NOT_AVAILABLE")
      .option(CL_MEM_COPY_OVERLAP, "CL_MEM_COPY_OVERLAP")
      .option(CL_IMAGE_FORMAT_MISMATCH, "CL_IMAGE_FORMAT_MISMATCH")
      .option(CL_IMAGE_FORMAT_NOT_SUPPORTED, "CL_IMAGE_FORMAT_NOT_SUPPORTED")
      .option(CL_BUILD_PROGRAM_FAILURE, "CL_BUILD_PROGRAM_FAILURE")
      .option(CL_MAP_FAILURE, "CL_MAP_FAILURE")
      .option(CL_MISALIGNED_SUB_BUFFER_OFFSET,
              "CL_MISALIGNED_SUB_BUFFER_OFFSET")
      .option(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
              "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST")
      .option(CL_COMPILE_PROGRAM_FAILURE, "CL_COMPILE_PROGRAM_FAILURE")
      .option(CL_LINKER_NOT_AVAILABLE, "CL_LINKER_NOT_AVAILABLE")
      .option(CL_LINK_PROGRAM_FAILURE, "CL_LINK_PROGRAM_FAILURE")
      .option(CL_DEVICE_PARTITION_FAILED, "CL_DEVICE_PARTITION_FAILED")
      .option(CL_KERNEL_ARG_INFO_NOT_AVAILABLE,
              "CL_KERNEL_ARG_INFO_NOT_AVAILABLE")
      .option(CL_INVALID_VALUE, "CL_INVALID_VALUE")
      .option(CL_INVALID_DEVICE_TYPE, "CL_INVALID_DEVICE_TYPE")
      .option(CL_INVALID_PLATFORM, "CL_INVALID_PLATFORM")
      .option(CL_INVALID_DEVICE, "CL_INVALID_DEVICE")
      .option(CL_INVALID_CONTEXT, "CL_INVALID_CONTEXT")
      .option(CL_INVALID_QUEUE_PROPERTIES, "CL_INVALID_QUEUE_PROPERTIES")
      .option(CL_INVALID_COMMAND_QUEUE, "CL_INVALID_COMMAND_QUEUE")
      .option(CL_INVALID_HOST_PTR, "CL_INVALID_HOST_PTR")
      .option(CL_INVALID_MEM_OBJECT, "CL_INVALID_MEM_OBJECT")
      .option(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR,
              "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR")
      .option(CL_INVALID_IMAGE_SIZE, "CL_INVALID_IMAGE_SIZE")
      .option(CL_INVALID_SAMPLER, "CL_INVALID_SAMPLER")
      .option(CL_INVALID_BINARY, "CL_INVALID_BINARY")
      .option(CL_INVALID_BUILD_OPTIONS, "CL_INVALID_BUILD_OPTIONS")
      .option(CL_INVALID_PROGRAM, "CL_INVALID_PROGRAM")
      .option(CL_INVALID_PROGRAM_EXECUTABLE, "CL_INVALID_PROGRAM_EXECUTABLE")
      .option(CL_INVALID_KERNEL_NAME, "CL_INVALID_KERNEL_NAME")
      .option(CL_INVALID_KERNEL_DEFINITION, "CL_INVALID_KERNEL_DEFINITION")
      .option(CL_INVALID_KERNEL, "CL_INVALID_KERNEL")
      .option(CL_INVALID_ARG_INDEX, "CL_INVALID_ARG_INDEX")
      .option(CL_INVALID_ARG_VALUE, "CL_INVALID_ARG_VALUE")
      .option(CL_INVALID_ARG_SIZE, "CL_INVALID_ARG_SIZE")
      .option(CL_INVALID_KERNEL_ARGS, "CL_INVALID_KERNEL_ARGS")
      .option(CL_INVALID_WORK_DIMENSION, "CL_INVALID_WORK_DIMENSION")
      .option(CL_INVALID_WORK_GROUP_SIZE, "CL_INVALID_WORK_GROUP_SIZE")
      .option(CL_INVALID_WORK_ITEM_SIZE, "CL_INVALID_WORK_ITEM_SIZE")
      .option(CL_INVALID_GLOBAL_OFFSET, "CL_INVALID_GLOBAL_OFFSET")
      .option(CL_INVALID_EVENT_WAIT_LIST, "CL_INVALID_EVENT_WAIT_LIST")
      .option(CL_INVALID_EVENT, "CL_INVALID_EVENT")
      .option(CL_INVALID_OPERATION, "CL_INVALID_OPERATION")
      .option(CL_INVALID_GL_OBJECT, "CL_INVALID_GL_OBJECT")
      .option(CL_INVALID_BUFFER_SIZE, "CL_INVALID_BUFFER_SIZE")
      .option(CL_INVALID_MIP_LEVEL, "CL_INVALID_MIP_LEVEL")
      .option(CL_INVALID_GLOBAL_WORK_SIZE, "CL_INVALID_GLOBAL_WORK_SIZE")
      .option(CL_INVALID_PROPERTY, "CL_INVALID_PROPERTY")
      .option(CL_INVALID_IMAGE_DESCRIPTOR, "CL_INVALID_IMAGE_DESCRIPTOR")
      .option(CL_INVALID_COMPILER_OPTIONS, "CL_INVALID_COMPILER_OPTIONS")
      .option(CL_INVALID_LINKER_OPTIONS, "CL_INVALID_LINKER_OPTIONS")
      .option(CL_INVALID_DEVICE_PARTITION_COUNT,
              "CL_INVALID_DEVICE_PARTITION_COUNT")
      .fallback("Unknown OpenCL error");
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Platform::all() -> std::vector<Platform> {
  cl_uint num_platforms = 0;
  auto status = clGetPlatformIDs(/*num_entries=*/0,
                                 /*platforms=*/nullptr,
                                 &num_platforms);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query number of the available OpenCL platforms ({}).",
             error_message(status));

  std::vector<cl_platform_id> platforms(num_platforms);
  status = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the available OpenCL platforms ({}).",
             error_message(status));

  return platforms |
         std::views::transform([](cl_platform_id p) { return Platform{p}; }) |
         std::ranges::to<std::vector>();
}

auto Platform::default_() -> Platform {
  /// @todo We shall come with a better heuristic here.
  const auto all_platforms = all();
  TIT_ENSURE(!all_platforms.empty(), "No OpenCL platforms found.");
  return all().front();
}

auto Platform::base() const noexcept -> cl_platform_id {
  TIT_ASSERT(platform_ != nullptr, "Platform pointer is null!");
  return platform_;
}

auto Platform::name() const -> std::string {
  size_t name_width = 0;
  auto status = clGetPlatformInfo(base(),
                                  CL_PLATFORM_NAME,
                                  /*param_value_size=*/0,
                                  /*param_value=*/nullptr,
                                  &name_width);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL platform name length ({}).",
             error_message(status));

  std::string name_str(name_width - 1, '\0');
  status = clGetPlatformInfo(base(),
                             CL_PLATFORM_NAME,
                             name_width,
                             name_str.data(),
                             /*param_value_size_ret=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL platform name ({}).",
             error_message(status));

  return name_str;
}

auto Platform::info() const -> std::string {
  StrHashMap<size_t> device_names_map{};
  for (const auto& device : Device::all(*this)) {
    device_names_map[device.name()] += 1;
  }
  return std::format(
      "{} ({})",
      name(),
      str_join(", ",
               std::views::transform(device_names_map, [](const auto& pair) {
                 if (pair.second == 1) return pair.first;
                 return std::format("{} × {}", pair.second, pair.first);
               })));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Device::Retainer_::operator()(cl_device_id device) const {
  const auto status = clRetainDevice(device);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not retain an OpenCL device reference ({}).",
             error_message(status));
}

void Device::Releaser_::operator()(cl_device_id device) const {
  const auto status = clReleaseDevice(device);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not release an OpenCL device reference ({}).",
             error_message(status));
}

auto Device::all(const Platform& platform, DeviceTypes types)
    -> std::vector<Device> {
  TIT_ASSERT(types != DeviceTypes::none, "No device types requested!");

  // OpenCL specifies also a "custom" device type. This must be a device with
  // incomplete OpenCL spec support, so we do not bother supporting such devices
  // for now.
  using enum DeviceTypes;
  cl_device_type device_types_ocl = 0;
  if (types & cpu) device_types_ocl |= CL_DEVICE_TYPE_CPU;
  if (types & gpu) device_types_ocl |= CL_DEVICE_TYPE_GPU;
  if (types & accelerator) device_types_ocl |= CL_DEVICE_TYPE_ACCELERATOR;
  if (types & default_) device_types_ocl |= CL_DEVICE_TYPE_DEFAULT;

  cl_uint num_devices = 0;
  auto status = clGetDeviceIDs(platform.base(),
                               device_types_ocl,
                               /*num_entries=*/0,
                               /*devices=*/nullptr,
                               &num_devices);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query number of OpenCL devices on platform '{}' ({}).",
             platform.name(),
             error_message(status));

  std::vector<cl_device_id> devices(num_devices);
  status = clGetDeviceIDs(platform.base(),
                          device_types_ocl,
                          num_devices,
                          devices.data(),
                          nullptr);
  TIT_ENSURE(
      status == CL_SUCCESS,
      "Could not query the available OpenCL devices on platform '{}' ({}).",
      platform.name(),
      error_message(status));

  return devices |
         std::views::transform([](cl_device_id d) { return Device{d}; }) |
         std::ranges::to<std::vector>();
}

auto Device::default_(const Platform& platform) -> Device {
  /// @todo We shall come with a better heuristic here.
  const auto default_devices = all(platform, DeviceTypes::default_);
  TIT_ENSURE(!default_devices.empty(),
             "Cannot find any default devices on OpenCL platform '{}'.",
             platform.name());
  return default_devices.front();
}

auto Device::base() const noexcept -> cl_device_id {
  TIT_ASSERT(device_ != nullptr, "Device pointer is null!");
  return device_.get();
}

auto Device::name() const -> std::string {
  size_t name_width = 0;
  auto status = clGetDeviceInfo(base(),
                                CL_DEVICE_NAME,
                                /*param_value_size=*/0,
                                /*param_value=*/nullptr,
                                &name_width);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL device name length ({}).",
             error_message(status));

  std::string name_str(name_width - 1, '\0');
  status = clGetDeviceInfo(base(),
                           CL_DEVICE_NAME,
                           name_width,
                           name_str.data(),
                           /*param_value_size_ret=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL device name ({}).",
             error_message(status));

  return name_str;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Context::Retainer_::operator()(cl_context context) const {
  const auto status = clRetainContext(context);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not retain a context reference ({}).",
             error_message(status));
}

void Context::Releaser_::operator()(cl_context context) const {
  const auto status = clReleaseContext(context);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not release a context reference ({}).",
             error_message(status));
}

Context::Context(const std::vector<Device>& devices) {
  const auto device_ids =
      devices | std::views::transform([](const auto& d) { return d.base(); }) |
      std::ranges::to<std::vector>();

  cl_int status = 0;
  context_.reset(clCreateContext(/*properties=*/nullptr,
                                 device_ids.size(),
                                 device_ids.data(),
                                 /*pfn_notify=*/nullptr,
                                 /*user_data=*/nullptr,
                                 &status));
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not create an OpenCL context on devices {} ({}).",
             devices | std::views::transform(
                           [](const auto& d) { return str_quote(d.name()); }),
             error_message(status));
}

auto Context::base() const noexcept -> cl_context {
  TIT_ASSERT(context_ != nullptr, "Context pointer is null!");
  return context_.get();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void CommandQueue::Retainer_::operator()(cl_command_queue queue) const {
  const auto status = clRetainCommandQueue(queue);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not retain a command queue reference ({}).",
             error_message(status));
}

void CommandQueue::Releaser_::operator()(cl_command_queue queue) const {
  const auto status = clReleaseCommandQueue(queue);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not release a command queue reference ({}).",
             error_message(status));
}

CommandQueue::CommandQueue(Context& context, const Device& device) {
  cl_int status = 0;
  queue_.reset(clCreateCommandQueue(context.base(),
                                    device.base(),
                                    /*properties=*/0,
                                    &status));
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not create an OpenCL command queue on device '{}' ({}).",
             device.name(),
             error_message(status));
}

auto CommandQueue::base() const noexcept -> cl_command_queue {
  TIT_ASSERT(queue_ != nullptr, "Command queue pointer is null!");
  return queue_.get();
}

void CommandQueue::flush() const {
  const auto status = clFlush(base());
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not flush an OpenCL command queue ({}).",
             error_message(status));
}

void CommandQueue::finish() const {
  const auto status = clFinish(base());
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not finish an OpenCL command queue ({}).",
             error_message(status));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void BaseMem::Retainer_::operator()(cl_mem mem) const {
  const auto status = clRetainMemObject(mem);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not retain an OpenCL memory object reference ({}).",
             error_message(status));
}

void BaseMem::Releaser_::operator()(cl_mem mem) const {
  const auto status = clReleaseMemObject(mem);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not release an OpenCL memory object reference ({}).",
             error_message(status));
}

BaseMem::BaseMem(Context& context,
                 BufferAccess access,
                 size_t width,
                 const byte_t* data) {
  TIT_ENSURE(width != 0, "Width must be non-zero!");
  TIT_ENSURE(access != BufferAccess::none, "Buffer access must be non-zero!");

  using enum BufferAccess;
  cl_mem_flags access_ocl = 0;
  if (access & host_read_write) access_ocl |= 0; // No flag is needed.
  else if (access & host_read) access_ocl |= CL_MEM_HOST_READ_ONLY;
  else if (access & host_write) access_ocl |= CL_MEM_HOST_WRITE_ONLY;
  else access_ocl |= CL_MEM_HOST_NO_ACCESS;
  if (access & device_read_write) access_ocl |= CL_MEM_READ_WRITE;
  else if (access & device_read) access_ocl |= CL_MEM_READ_ONLY;
  else if (access & device_write) access_ocl |= CL_MEM_WRITE_ONLY;
  else TIT_ASSERT(false, "No device access flags are set!");

  if (data != nullptr) access_ocl |= CL_MEM_COPY_HOST_PTR;

  cl_int status = 0;
  mem_.reset(clCreateBuffer(context.base(),
                            access_ocl,
                            width,
                            const_cast<byte_t*>(data), // NOLINT(*-const-cast)
                            &status));
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not create an OpenCL buffer of size {} ({}).",
             width,
             error_message(status));
}

auto BaseMem::base() const noexcept -> cl_mem {
  TIT_ASSERT(mem_ != nullptr, "Memory object pointer is null!");
  return mem_.get();
}

auto BaseMem::width() const -> size_t {
  size_t width = 0;
  auto status = clGetMemObjectInfo(base(),
                                   CL_MEM_SIZE,
                                   sizeof(width),
                                   &width,
                                   /*param_value_size_ret=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL buffer size ({}).",
             error_message(status));

  return width;
}

void BaseMem::enqueue_read_(CommandQueue& queue, std::span<byte_t> data) const {
  const auto status = clEnqueueReadBuffer(queue.base(),
                                          base(),
                                          /*blocking_read=*/CL_TRUE,
                                          /*offset=*/0,
                                          data.size(),
                                          data.data(),
                                          /*num_events_in_wait_list=*/0,
                                          /*event_wait_list=*/nullptr,
                                          /*event=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not enqueue a read operation from OpenCL buffer ({}).",
             error_message(status));
}

void BaseMem::enqueue_write_(CommandQueue& queue,
                             std::span<const byte_t> data) {
  const auto status = clEnqueueWriteBuffer(queue.base(),
                                           base(),
                                           /*blocking_write=*/CL_TRUE,
                                           /*offset=*/0,
                                           data.size(),
                                           data.data(),
                                           /*num_events_in_wait_list=*/0,
                                           /*event_wait_list=*/nullptr,
                                           /*event=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not enqueue a write operation from OpenCL buffer ({}).",
             error_message(status));

  // Suppress clang-tidy's desire to make this function const.
  static_cast<void>(this);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Program::Retainer_::operator()(cl_program program) const {
  const auto status = clRetainProgram(program);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not retain a program reference ({}).",
             error_message(status));
}

void Program::Releaser_::operator()(cl_program program) const {
  const auto status = clReleaseProgram(program);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not release a program reference ({}).",
             error_message(status));
}

/// @todo This is a temporary implementation.
Program::Program(Context& context, const Device& device, CStrView source) {
  // OpenCL tends to cache the program binary, if the source is the same.
  // This leads to a problem when the `#include`-ed files are modified,
  // and the program needs to be recompiled. This is supposed to be fixed
  // by using separate program objects for headers (see `clBuildProgram` and
  // `clCreateProgramWithSource`). However, these two functions are bogus on
  // macOS (and Apple has no plans to fix it), so we'll just disable caching
  // for now by adding a random string to the source code.
  const auto randomizer = std::format("\n\n// {} \n", std::random_device{}());

  cl_int status = 0;
  std::array source_pointers{source.c_str(), randomizer.c_str()};
  const std::array source_sizes{source.size(), randomizer.size()};
  program_.reset(clCreateProgramWithSource(context.base(),
                                           source_pointers.size(),
                                           source_pointers.data(),
                                           source_sizes.data(),
                                           &status));
  TIT_ENSURE(
      status == CL_SUCCESS,
      "Could not create an OpenCL program from source ({}): ```cl\n{}```.",
      status,
      source);

  cl_device_id device_id = device.base();
  status = clBuildProgram(
      base(),
      /*num_devices=*/1,
      &device_id,
      /*options=*/
      std::format("-I {} "
                  "-Werror "
                  "-cl-mad-enable "
                  "-cl-fp32-correctly-rounded-divide-sqrt "
                  "-cl-unsafe-math-optimizations",
                  (exe_path().parent_path().parent_path() / "opencl").string())
          .c_str(),
      /*pfn_notify=*/nullptr,
      /*user_data=*/nullptr);
  if (status != CL_SUCCESS) {
    std::array<char, 4096> log_buffer{};
    size_t log_size = 0;
    clGetProgramBuildInfo(base(),
                          device_id,
                          CL_PROGRAM_BUILD_LOG,
                          log_buffer.size() - 1,
                          log_buffer.data(),
                          &log_size);
    log_buffer[log_size] = '\0';
    TIT_THROW("clBuildProgram failed: {}\n{}",
              error_message(status),
              log_buffer.data());
  }
}

auto Program::base() const noexcept -> cl_program {
  TIT_ASSERT(program_ != nullptr, "Program pointer is null!");
  return program_.get();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Kernel::Retainer_::operator()(cl_kernel kernel) const {
  const auto status = clRetainKernel(kernel);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not retain a kernel reference ({}).",
             error_message(status));
}

void Kernel::Releaser_::operator()(cl_kernel kernel) const {
  const auto status = clReleaseKernel(kernel);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not release a kernel reference ({}).",
             error_message(status));
}

Kernel::Kernel(const Program& program, CStrView name) {
  cl_int status = 0;
  kernel_.reset(clCreateKernel(program.base(), name.c_str(), &status));
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not create an OpenCL kernel '{}' ({}).",
             name,
             error_message(status));
}

auto Kernel::base() const noexcept -> cl_kernel {
  TIT_ASSERT(kernel_ != nullptr, "Kernel pointer is null!");
  return kernel_.get();
}

auto Kernel::name() const -> std::string {
  size_t name_width = 0;
  auto status = clGetKernelInfo(base(),
                                CL_KERNEL_FUNCTION_NAME,
                                /*param_value_size=*/0,
                                /*param_value=*/nullptr,
                                &name_width);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL kernel name length ({}).",
             error_message(status));

  std::string name_str(name_width - 1, '\0');
  status = clGetKernelInfo(base(),
                           CL_KERNEL_FUNCTION_NAME,
                           name_width,
                           name_str.data(),
                           /*param_value_size_ret=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the OpenCL kernel name ({}).",
             error_message(status));

  return name_str;
}

auto Kernel::num_args() const -> size_t {
  size_t num_args = 0;
  auto status = clGetKernelInfo(base(),
                                CL_KERNEL_NUM_ARGS,
                                /*param_value_size=*/0,
                                /*param_value=*/nullptr,
                                &num_args);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not query the number of OpenCL '{}' kernel arguments ({}).",
             name(),
             error_message(status));

  return num_args;
}

void Kernel::set_arg_bytes_(size_t index, std::span<const byte_t> data) {
  TIT_ASSERT(index < num_args(), "Kernel argument index is out of range!");

  const auto status = clSetKernelArg(base(),
                                     static_cast<cl_uint>(index),
                                     data.size(),
                                     data.data());
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not set OpenCL '{}' kernel argument {} ({}).",
             name(),
             index,
             error_message(status));

  // Suppress clang-tidy's desire to make this function const.
  static_cast<void>(this);
}

void Kernel::enqueue_exec(CommandQueue& queue,
                          std::span<const size_t> global_work_offset,
                          std::span<const size_t> global_work_size,
                          std::span<const size_t> local_work_size) const {
  TIT_ASSERT(!global_work_size.empty(),
             "Global work offset must not be empty!");
  const auto dim = global_work_size.size();
  TIT_ASSERT(global_work_offset.empty() || global_work_offset.size() == dim,
             "Global work offset has the wrong number of dimensions!");
  TIT_ASSERT(local_work_size.empty() || local_work_size.size() == dim,
             "Local work size has the wrong number of dimensions!");

  constexpr auto data_or_nullptr = [](auto span) {
    return span.empty() ? nullptr : span.data();
  };

  const auto status = clEnqueueNDRangeKernel( //
      queue.base(),
      base(),
      dim,
      data_or_nullptr(global_work_offset),
      data_or_nullptr(global_work_size),
      data_or_nullptr(local_work_size),
      /*num_events_in_wait_list=*/0,
      /*event_wait_list=*/nullptr,
      /*event=*/nullptr);
  TIT_ENSURE(status == CL_SUCCESS,
             "Could not enqueue OpenCL kernel '{}' ({}).",
             name(),
             error_message(status));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ocl
