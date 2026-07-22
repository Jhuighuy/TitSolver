/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <H5Apublic.h>
#include <H5Dpublic.h>
#include <H5FDmpi.h>
#include <H5FDmpio.h>
#include <H5Fpublic.h>
#include <H5Gpublic.h>
#include <H5Ipublic.h>
#include <H5Ppublic.h>
#include <H5Spublic.h>
#include <H5Tpublic.h>
#include <H5public.h>
#include <mpi.h>

#include "tit/core/exception.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/mpi.hpp"
#include "tit/io/parallel_run.hpp"
#include "tit/io/run.hpp"
#include "tit/io/run_backend.hpp"
#include "tit/io/type.hpp"

// HDF5 and OpenMPI expose predefined handles as stateful macros. Static
// analysis of those third-party macro expansions produces false positives.
// NOLINTBEGIN(readability-simplify-boolean-expr,bugprone-casting-through-void)

namespace tit::io {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void check_hdf5(herr_t status, std::string_view operation) {
  TIT_ENSURE(status >= 0, "{} failed.", operation);
}

auto checked_hdf5_id(hid_t id, std::string_view operation) -> hid_t {
  TIT_ENSURE(id >= 0, "{} failed.", operation);
  return id;
}

template<herr_t (*Close)(hid_t)>
class HDF5Handle final {
public:

  HDF5Handle() noexcept = default;
  explicit HDF5Handle(hid_t id) noexcept : id_{id} {}

  HDF5Handle(const HDF5Handle&) = delete;
  auto operator=(const HDF5Handle&) -> HDF5Handle& = delete;

  HDF5Handle(HDF5Handle&& other) noexcept : id_{std::exchange(other.id_, -1)} {}

  auto operator=(HDF5Handle&& other) noexcept -> HDF5Handle& {
    if (this == &other) return *this;
    reset();
    id_ = std::exchange(other.id_, -1);
    return *this;
  }

  ~HDF5Handle() {
    reset();
  }

  void reset() noexcept {
    if (id_ < 0) return;
    static_cast<void>(Close(id_));
    id_ = -1;
  }

  auto get() const noexcept -> hid_t {
    return id_;
  }

private:

  hid_t id_ = -1;

}; // class HDF5Handle

using HDF5File = HDF5Handle<H5Fclose>;
using HDF5Group = HDF5Handle<H5Gclose>;
using HDF5DataSet = HDF5Handle<H5Dclose>;
using HDF5DataSpace = HDF5Handle<H5Sclose>;
using HDF5PropertyList = HDF5Handle<H5Pclose>;
using HDF5Attribute = HDF5Handle<H5Aclose>;

auto partial_path(const std::filesystem::path& path) -> std::filesystem::path {
  auto partial = path;
  partial += ".partial";
  return partial;
}

auto hdf5_type(Kind kind) -> hid_t {
  using enum Kind::ID;
  switch (kind.id()) {
    case int8:    return H5T_NATIVE_INT8;
    case uint8:   return H5T_NATIVE_UINT8;
    case int16:   return H5T_NATIVE_INT16;
    case uint16:  return H5T_NATIVE_UINT16;
    case int32:   return H5T_NATIVE_INT32;
    case uint32:  return H5T_NATIVE_UINT32;
    case int64:   return H5T_NATIVE_INT64;
    case uint64:  return H5T_NATIVE_UINT64;
    case float32: return H5T_NATIVE_FLOAT;
    case float64: return H5T_NATIVE_DOUBLE;
    default:      std::unreachable();
  }
}

template<class Value>
void write_attribute(hid_t object,
                     std::string_view name,
                     hid_t datatype,
                     const Value& value) {
  const HDF5DataSpace space{
      checked_hdf5_id(H5Screate(H5S_SCALAR), "H5Screate")};
  const HDF5Attribute attribute{
      checked_hdf5_id(H5Acreate2(object,
                                 std::string{name}.c_str(),
                                 datatype,
                                 space.get(),
                                 H5P_DEFAULT,
                                 H5P_DEFAULT),
                      "H5Acreate2")};
  check_hdf5(H5Awrite(attribute.get(), datatype, &value), "H5Awrite");
}

auto field_dimensions(std::uint64_t size, Type type) -> std::vector<hsize_t> {
  TIT_ENSURE(size <= std::numeric_limits<hsize_t>::max(),
             "HDF5 dataset is too large.");
  std::vector<hsize_t> dimensions{static_cast<hsize_t>(size)};
  switch (type.rank()) {
    case Rank::scalar: break;
    case Rank::vector:
      dimensions.push_back(static_cast<hsize_t>(type.dim()));
      break;
    case Rank::matrix:
      dimensions.push_back(static_cast<hsize_t>(type.dim()));
      dimensions.push_back(static_cast<hsize_t>(type.dim()));
      break;
    default: std::unreachable();
  }
  return dimensions;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

namespace impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class ParallelRunWriterState final :
    public std::enable_shared_from_this<ParallelRunWriterState> {
public:

  ParallelRunWriterState(std::filesystem::path path,
                         RunMetadata metadata,
                         dist::Communicator communicator)
      : path_{std::move(path)}, communicator_{std::move(communicator)} {
    if (communicator_.rank() == 0) {
      publisher_ = make_run_publisher(path_, std::move(metadata));
    }
    communicator_.barrier();
  }

  auto begin(std::uint64_t step, double time)
      -> std::shared_ptr<ParallelFrameWriterState>;

  auto communicator() const noexcept -> const dist::Communicator& {
    return communicator_;
  }

  auto publisher() const noexcept -> const std::shared_ptr<RunPublisherState>& {
    return publisher_;
  }

  auto path() const noexcept -> const std::filesystem::path& {
    return path_;
  }

private:

  std::filesystem::path path_;
  dist::Communicator communicator_;
  std::shared_ptr<RunPublisherState> publisher_;

}; // class ParallelRunWriterState

class ParallelFrameWriterState final {
public:

  ParallelFrameWriterState(std::shared_ptr<ParallelRunWriterState> run,
                           FramePublication publication)
      : run_{std::move(run)}, publication_{std::move(publication)} {
    const HDF5PropertyList access{
        checked_hdf5_id(H5Pcreate(H5P_FILE_ACCESS),
                        "H5Pcreate(H5P_FILE_ACCESS)")};
    check_hdf5(
        H5Pset_fapl_mpio(access.get(),
                         dist::MPICommunicatorAccess::get(run_->communicator()),
                         MPI_INFO_NULL),
        "H5Pset_fapl_mpio");
    file_ = HDF5File{
        checked_hdf5_id(H5Fcreate(publication_.partial_path().string().c_str(),
                                  H5F_ACC_TRUNC,
                                  H5P_DEFAULT,
                                  access.get()),
                        "H5Fcreate")};

    write_attribute(file_.get(),
                    "format_version",
                    H5T_NATIVE_UINT32,
                    run_format_version);
    write_attribute(file_.get(),
                    "step",
                    H5T_NATIVE_UINT64,
                    publication_.descriptor().step());
    write_attribute(file_.get(),
                    "time",
                    H5T_NATIVE_DOUBLE,
                    publication_.descriptor().time());
    particles_ = HDF5Group{checked_hdf5_id(H5Gcreate2(file_.get(),
                                                      "particles",
                                                      H5P_DEFAULT,
                                                      H5P_DEFAULT,
                                                      H5P_DEFAULT),
                                           "H5Gcreate2(particles)")};
    fields_ = HDF5Group{checked_hdf5_id(H5Gcreate2(file_.get(),
                                                   "fields",
                                                   H5P_DEFAULT,
                                                   H5P_DEFAULT,
                                                   H5P_DEFAULT),
                                        "H5Gcreate2(fields)")};
  }

  ParallelFrameWriterState(const ParallelFrameWriterState&) = delete;
  ParallelFrameWriterState(ParallelFrameWriterState&&) = delete;
  auto operator=(const ParallelFrameWriterState&)
      -> ParallelFrameWriterState& = delete;
  auto operator=(ParallelFrameWriterState&&)
      -> ParallelFrameWriterState& = delete;

  ~ParallelFrameWriterState() {
    close_();
    if (!committed_ && run_->communicator().rank() == 0) {
      abandon_frame(run_->publisher());
    }
  }

  void write(std::string_view name,
             Type type,
             std::span<const std::byte> local_data,
             std::size_t local_size) {
    TIT_ENSURE(!committed_, "Frame is already committed.");
    TIT_ENSURE(file_.get() >= 0, "Parallel frame file is closed.");
    TIT_ENSURE(!name.empty() && !name.contains('/'),
               "Invalid field name '{}'.",
               name);
    TIT_ENSURE(local_data.size() == local_size * type.width(),
               "Field '{}' byte size does not match its type.",
               name);
    TIT_ENSURE(std::ranges::none_of(
                   field_descriptors_,
                   [&](const auto& field) { return field.name() == name; }),
               "Duplicate field '{}'.",
               name);
    if (local_particle_count_.has_value()) {
      TIT_ENSURE(*local_particle_count_ == local_size,
                 "Local field '{}' has a different particle count.",
                 name);
    } else {
      local_particle_count_ = local_size;
    }

    const auto local_count = static_cast<std::uint64_t>(local_size);
    const auto global_count = run_->communicator().all_reduce_sum(local_count);
    const auto global_offset =
        run_->communicator().exclusive_scan_sum(local_count);
    TIT_ENSURE(global_count <= std::numeric_limits<std::size_t>::max(),
               "Global field '{}' is too large for this platform.",
               name);
    if (global_particle_count_.has_value()) {
      TIT_ENSURE(*global_particle_count_ == global_count,
                 "Global field '{}' has a different particle count.",
                 name);
    } else {
      global_particle_count_ = global_count;
    }

    auto global_dimensions = field_dimensions(global_count, type);
    const HDF5DataSpace file_space{checked_hdf5_id(
        H5Screate_simple(static_cast<int>(global_dimensions.size()),
                         global_dimensions.data(),
                         nullptr),
        "H5Screate_simple(file)")};
    const auto group =
        name == "id" || name == "kind" ? particles_.get() : fields_.get();
    const HDF5DataSet dataset{
        checked_hdf5_id(H5Dcreate2(group,
                                   std::string{name}.c_str(),
                                   hdf5_type(type.kind()),
                                   file_space.get(),
                                   H5P_DEFAULT,
                                   H5P_DEFAULT,
                                   H5P_DEFAULT),
                        "H5Dcreate2")};
    write_attribute(dataset.get(), "type_id", H5T_NATIVE_UINT32, type.id());
    write_attribute(dataset.get(), "size", H5T_NATIVE_UINT64, global_count);

    auto local_dimensions = global_dimensions;
    local_dimensions.front() = static_cast<hsize_t>(local_count);
    HDF5DataSpace memory_space;
    if (local_count == 0) {
      check_hdf5(H5Sselect_none(file_space.get()), "H5Sselect_none");
      memory_space = HDF5DataSpace{
          checked_hdf5_id(H5Screate(H5S_NULL), "H5Screate(H5S_NULL)")};
    } else {
      std::vector<hsize_t> start(global_dimensions.size(), 0);
      start.front() = static_cast<hsize_t>(global_offset);
      check_hdf5(H5Sselect_hyperslab(file_space.get(),
                                     H5S_SELECT_SET,
                                     start.data(),
                                     nullptr,
                                     local_dimensions.data(),
                                     nullptr),
                 "H5Sselect_hyperslab");
      memory_space = HDF5DataSpace{checked_hdf5_id(
          H5Screate_simple(static_cast<int>(local_dimensions.size()),
                           local_dimensions.data(),
                           nullptr),
          "H5Screate_simple(memory)")};
    }

    const HDF5PropertyList transfer{
        checked_hdf5_id(H5Pcreate(H5P_DATASET_XFER),
                        "H5Pcreate(H5P_DATASET_XFER)")};
    check_hdf5(H5Pset_dxpl_mpio(transfer.get(), H5FD_MPIO_COLLECTIVE),
               "H5Pset_dxpl_mpio");
    check_hdf5(H5Dwrite(dataset.get(),
                        hdf5_type(type.kind()),
                        memory_space.get(),
                        file_space.get(),
                        transfer.get(),
                        local_data.empty() ? nullptr : local_data.data()),
               "H5Dwrite");

    field_descriptors_.emplace_back(std::string{name},
                                    type,
                                    static_cast<std::size_t>(global_count));
  }

  void commit() {
    TIT_ENSURE(!committed_, "Frame is already committed.");
    TIT_ENSURE(!field_descriptors_.empty(), "Cannot publish an empty frame.");
    close_();
    run_->communicator().barrier();
    if (run_->communicator().rank() == 0) {
      publish_frame(run_->publisher(), publication_, field_descriptors_);
    }
    run_->communicator().barrier();
    committed_ = true;
  }

private:

  void close_() noexcept {
    fields_.reset();
    particles_.reset();
    file_.reset();
  }

  std::shared_ptr<ParallelRunWriterState> run_;
  FramePublication publication_;
  HDF5File file_;
  HDF5Group particles_;
  HDF5Group fields_;
  std::vector<FieldDescriptor> field_descriptors_;
  std::optional<std::size_t> local_particle_count_;
  std::optional<std::uint64_t> global_particle_count_;
  bool committed_ = false;

}; // class ParallelFrameWriterState

auto ParallelRunWriterState::begin(std::uint64_t step, double time)
    -> std::shared_ptr<ParallelFrameWriterState> {
  TIT_ENSURE(std::isfinite(time), "Frame time must be finite.");
  const auto final = path_ / "frames" / std::format("frame-{:08}.h5", step);
  auto publication =
      FramePublication{FrameDescriptor{step, time}, partial_path(final), final};
  if (communicator_.rank() == 0) {
    publication = begin_frame_publication(publisher_, step, time);
  }
  communicator_.barrier();
  return std::make_shared<ParallelFrameWriterState>(shared_from_this(),
                                                    std::move(publication));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace impl

ParallelFrameWriter::ParallelFrameWriter(
    std::shared_ptr<impl::ParallelFrameWriterState> state) noexcept
    : state_{std::move(state)} {}

ParallelFrameWriter::~ParallelFrameWriter() = default;

void ParallelFrameWriter::write_(std::string_view name,
                                 Type type,
                                 std::span<const std::byte> local_data,
                                 std::size_t local_size) {
  TIT_ENSURE(state_ != nullptr, "Parallel frame writer is null.");
  state_->write(name, type, local_data, local_size);
}

void ParallelFrameWriter::commit() {
  TIT_ENSURE(state_ != nullptr, "Parallel frame writer is null.");
  state_->commit();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ParallelRunWriter::ParallelRunWriter(std::filesystem::path path,
                                     RunMetadata metadata,
                                     dist::Communicator communicator)
    : state_{std::make_shared<impl::ParallelRunWriterState>(
          std::move(path),
          std::move(metadata),
          std::move(communicator))} {}

auto ParallelRunWriter::begin_frame(std::uint64_t step, double time)
    -> ParallelFrameWriter {
  return ParallelFrameWriter{state_->begin(step, time)};
}

auto ParallelRunWriter::path() const -> const std::filesystem::path& {
  return state_->path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io

// NOLINTEND(readability-simplify-boolean-expr,bugprone-casting-through-void)
