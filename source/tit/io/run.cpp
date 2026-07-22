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
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <highfive/H5DataSpace.hpp>
#include <highfive/H5File.hpp>
#include <highfive/H5Group.hpp>
#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include <nlohmann/json_fwd.hpp>

#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/type.hpp"
#include "tit/io/run.hpp"
#include "tit/io/type.hpp"

namespace tit::io {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using JSON = nlohmann::ordered_json;

auto partial_path(const std::filesystem::path& path) -> std::filesystem::path {
  auto partial = path;
  partial += ".partial";
  return partial;
}

void write_json_atomic(const std::filesystem::path& path, const JSON& json) {
  const auto partial = partial_path(path);
  std::ofstream stream{partial, std::ios::out | std::ios::trunc};
  TIT_ENSURE(stream.is_open(), "Unable to open '{}'.", partial.string());
  stream << json.dump(2) << '\n';
  stream.flush();
  TIT_ENSURE(stream.good(), "Unable to write '{}'.", partial.string());
  stream.close();

  std::error_code error;
  std::filesystem::rename(partial, path, error);
  TIT_ENSURE(!error,
             "Unable to publish '{}': {}.",
             path.string(),
             error.message());
}

auto read_json(const std::filesystem::path& path) -> JSON {
  std::ifstream stream{path};
  TIT_ENSURE(stream.is_open(), "Unable to open '{}'.", path.string());
  try {
    return JSON::parse(stream);
  } catch (const JSON::exception& error) {
    TIT_THROW("Unable to parse '{}': {}.", path.string(), error.what());
  }
}

auto read_metadata(const std::filesystem::path& path) -> RunMetadata {
  TIT_ENSURE(std::filesystem::is_directory(path),
             "Run path '{}' is not a directory.",
             path.string());
  const auto manifest = read_json(path / "manifest.json");
  TIT_ENSURE(manifest.at("format") == "bluetit-run", "Unsupported run format.");
  TIT_ENSURE(manifest.at("version") == run_format_version,
             "Unsupported run-format version.");
  return RunMetadata{manifest.at("name").get<std::string>(),
                     manifest.at("dimension").get<std::size_t>()};
}

auto field_dimensions(std::size_t size, Type type) -> std::vector<std::size_t> {
  std::vector<std::size_t> dimensions{size};
  switch (type.rank()) {
    case Rank::scalar: break;
    case Rank::vector: dimensions.push_back(type.dim()); break;
    case Rank::matrix:
      dimensions.push_back(type.dim());
      dimensions.push_back(type.dim());
      break;
    default: std::unreachable();
  }
  return dimensions;
}

template<class Val>
void write_dataset(HighFive::Group& group,
                   std::string_view name,
                   const HighFive::DataSpace& space,
                   std::span<const std::byte> data) {
  group.createDataSet<Val>(std::string{name}, space)
      .write_raw(safe_bit_ptr_cast<const Val*>(data.data()));
}

void write_dataset(HighFive::Group& group,
                   std::string_view name,
                   Type type,
                   std::span<const std::byte> data,
                   std::size_t size) {
  const HighFive::DataSpace space{field_dimensions(size, type)};
  using enum Kind::ID;
  switch (type.kind().id()) {
    case int8:    write_dataset<std::int8_t>(group, name, space, data); break;
    case uint8:   write_dataset<std::uint8_t>(group, name, space, data); break;
    case int16:   write_dataset<std::int16_t>(group, name, space, data); break;
    case uint16:  write_dataset<std::uint16_t>(group, name, space, data); break;
    case int32:   write_dataset<std::int32_t>(group, name, space, data); break;
    case uint32:  write_dataset<std::uint32_t>(group, name, space, data); break;
    case int64:   write_dataset<std::int64_t>(group, name, space, data); break;
    case uint64:  write_dataset<std::uint64_t>(group, name, space, data); break;
    case float32: write_dataset<float32_t>(group, name, space, data); break;
    case float64: write_dataset<float64_t>(group, name, space, data); break;
    default:      std::unreachable();
  }

  auto dataset = group.getDataSet(std::string{name});
  dataset.createAttribute("type_id", type.id());
  dataset.createAttribute("size", static_cast<std::uint64_t>(size));
}

auto read_descriptor(const HighFive::DataSet& dataset, std::string name)
    -> FieldDescriptor {
  const auto type_id = dataset.getAttribute("type_id").read<std::uint32_t>();
  const auto size = dataset.getAttribute("size").read<std::uint64_t>();
  TIT_ENSURE(size <= std::numeric_limits<std::size_t>::max(),
             "Field '{}' is too large for this platform.",
             name);
  return FieldDescriptor{std::move(name),
                         Type{type_id},
                         static_cast<std::size_t>(size)};
}

template<class Val>
void read_dataset(const HighFive::DataSet& dataset, std::span<std::byte> data) {
  dataset.read_raw(safe_bit_ptr_cast<Val*>(data.data()));
}

void read_dataset(const HighFive::DataSet& dataset,
                  Type type,
                  std::span<std::byte> data) {
  using enum Kind::ID;
  switch (type.kind().id()) {
    case int8:    read_dataset<std::int8_t>(dataset, data); break;
    case uint8:   read_dataset<std::uint8_t>(dataset, data); break;
    case int16:   read_dataset<std::int16_t>(dataset, data); break;
    case uint16:  read_dataset<std::uint16_t>(dataset, data); break;
    case int32:   read_dataset<std::int32_t>(dataset, data); break;
    case uint32:  read_dataset<std::uint32_t>(dataset, data); break;
    case int64:   read_dataset<std::int64_t>(dataset, data); break;
    case uint64:  read_dataset<std::uint64_t>(dataset, data); break;
    case float32: read_dataset<float32_t>(dataset, data); break;
    case float64: read_dataset<float64_t>(dataset, data); break;
    default:      std::unreachable();
  }
}

auto schema_json(const std::vector<FieldDescriptor>& fields) -> JSON {
  auto schema = JSON::array();
  for (const auto& field : fields) {
    schema.push_back({{"name", field.name()}, {"type", field.type().id()}});
  }
  return schema;
}

auto same_schema(const std::vector<FieldDescriptor>& left,
                 const std::vector<FieldDescriptor>& right) -> bool {
  return std::ranges::equal(left, right, [](const auto& a, const auto& b) {
    return a.name() == b.name() && a.type() == b.type();
  });
}

class PartialDirectoryGuard final {
public:

  explicit PartialDirectoryGuard(std::filesystem::path path)
      : path_{std::move(path)} {}

  PartialDirectoryGuard(const PartialDirectoryGuard&) = delete;
  PartialDirectoryGuard(PartialDirectoryGuard&&) = delete;
  auto operator=(const PartialDirectoryGuard&)
      -> PartialDirectoryGuard& = delete;
  auto operator=(PartialDirectoryGuard&&) -> PartialDirectoryGuard& = delete;

  ~PartialDirectoryGuard() {
    if (!active_) return;
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  void release() noexcept {
    active_ = false;
  }

private:

  std::filesystem::path path_;
  bool active_ = true;

}; // class PartialDirectoryGuard

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

namespace impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class RunWriterState final :
    public std::enable_shared_from_this<RunWriterState> {
public:

  RunWriterState(std::filesystem::path path, RunMetadata metadata)
      : path_{std::move(path)}, metadata_{std::move(metadata)} {
    TIT_ENSURE(metadata_.dimension() > 0 && metadata_.dimension() <= 3,
               "Run dimension must be between one and three.");
    if (std::filesystem::exists(path_)) {
      TIT_ENSURE(std::filesystem::is_directory(path_),
                 "Run path '{}' is not a directory.",
                 path_.string());
      TIT_ENSURE(std::filesystem::is_empty(path_),
                 "Run directory '{}' is not empty.",
                 path_.string());
    }
    std::filesystem::create_directories(path_ / "frames");
    std::filesystem::create_directories(path_ / "checkpoints");

    manifest_ = {
        {"format", "bluetit-run"},
        {"version", run_format_version},
        {"name", metadata_.name()},
        {"dimension", metadata_.dimension()},
        {"fields", JSON::array()},
    };
    index_ = {
        {"version", run_format_version},
        {"frames", JSON::array()},
        {"checkpoints", JSON::array()},
    };
    write_json_atomic(path_ / "manifest.json", manifest_);
    write_json_atomic(path_ / "index.json", index_);
  }

  auto begin(std::uint64_t step, double time)
      -> std::shared_ptr<FrameWriterState>;

  void publish(const std::filesystem::path& partial,
               const std::filesystem::path& final,
               FrameDescriptor descriptor,
               const std::vector<FieldDescriptor>& fields) {
    TIT_ENSURE(frame_active_, "No frame is active.");
    TIT_ENSURE(!fields.empty(), "Cannot publish an empty frame.");
    const auto has_id = std::ranges::any_of(fields, [](const auto& field) {
      return field.name() == "id";
    });
    const auto has_position =
        std::ranges::any_of(fields, [](const auto& field) {
          return field.name() == "r";
        });
    TIT_ENSURE(has_id && has_position,
               "A particle frame must contain 'id' and 'r' fields.");

    if (schema_.empty()) {
      schema_ = fields;
      manifest_["fields"] = schema_json(schema_);
      write_json_atomic(path_ / "manifest.json", manifest_);
    } else {
      TIT_ENSURE(same_schema(schema_, fields),
                 "Frame field schema differs from the run schema.");
    }

    std::error_code error;
    std::filesystem::rename(partial, final, error);
    TIT_ENSURE(!error,
               "Unable to publish frame '{}': {}.",
               final.string(),
               error.message());

    index_["frames"].push_back({
        {"step", descriptor.step()},
        {"time", descriptor.time()},
        {"file", std::filesystem::relative(final, path_).generic_string()},
    });
    write_json_atomic(path_ / "index.json", index_);
    last_frame_ = descriptor;
    frame_active_ = false;
  }

  void abandon() noexcept {
    frame_active_ = false;
  }

  auto path() const noexcept -> const std::filesystem::path& {
    return path_;
  }

private:

  std::filesystem::path path_;
  RunMetadata metadata_;
  JSON manifest_;
  JSON index_;
  std::vector<FieldDescriptor> schema_;
  std::optional<FrameDescriptor> last_frame_;
  bool frame_active_ = false;

}; // class RunWriterState

class FrameWriterState final {
  class PartialFrameGuard final {
  public:

    PartialFrameGuard(std::shared_ptr<RunWriterState> run,
                      std::filesystem::path partial)
        : run_{std::move(run)}, partial_{std::move(partial)} {}

    PartialFrameGuard(const PartialFrameGuard&) = delete;
    PartialFrameGuard(PartialFrameGuard&&) = delete;
    auto operator=(const PartialFrameGuard&) -> PartialFrameGuard& = delete;
    auto operator=(PartialFrameGuard&&) -> PartialFrameGuard& = delete;

    ~PartialFrameGuard() {
      if (!active_) return;
      std::error_code error;
      std::filesystem::remove(partial_, error);
      run_->abandon();
    }

    void release() noexcept {
      active_ = false;
    }

  private:

    std::shared_ptr<RunWriterState> run_;
    std::filesystem::path partial_;
    bool active_ = true;

  }; // class PartialFrameGuard

public:

  FrameWriterState(std::shared_ptr<RunWriterState> run,
                   FrameDescriptor descriptor,
                   std::filesystem::path partial,
                   std::filesystem::path final)
      : run_{std::move(run)}, guard_{run_, partial}, descriptor_{descriptor},
        partial_{std::move(partial)}, final_{std::move(final)},
        file_{std::make_unique<HighFive::File>(partial_.string(),
                                               HighFive::File::Overwrite)} {
    file_->createAttribute("format_version", run_format_version);
    file_->createAttribute("step", descriptor_.step());
    file_->createAttribute("time", descriptor_.time());
    fields_ = file_->createGroup("fields");
  }

  void write(std::string_view name,
             Type type,
             std::span<const std::byte> data,
             std::size_t size) {
    TIT_ENSURE(!committed_, "Frame is already committed.");
    TIT_ENSURE(!name.empty() && !name.contains('/'),
               "Invalid field name '{}'.",
               name);
    TIT_ENSURE(data.size() == size * type.width(),
               "Field '{}' byte size does not match its type.",
               name);
    TIT_ENSURE(std::ranges::none_of(
                   field_descriptors_,
                   [&](const auto& field) { return field.name() == name; }),
               "Duplicate field '{}'.",
               name);
    if (particle_count_.has_value()) {
      TIT_ENSURE(*particle_count_ == size,
                 "Field '{}' has a different particle count.",
                 name);
    } else {
      particle_count_ = size;
    }

    write_dataset(fields_, name, type, data, size);
    field_descriptors_.emplace_back(std::string{name}, type, size);
  }

  void commit() {
    TIT_ENSURE(!committed_, "Frame is already committed.");
    TIT_ENSURE(file_ != nullptr, "Frame file is already closed.");
    file_->flush();
    fields_ = {};
    file_.reset();
    run_->publish(partial_, final_, descriptor_, field_descriptors_);
    guard_.release();
    committed_ = true;
  }

private:

  std::shared_ptr<RunWriterState> run_;
  PartialFrameGuard guard_;
  FrameDescriptor descriptor_;
  std::filesystem::path partial_;
  std::filesystem::path final_;
  std::unique_ptr<HighFive::File> file_;
  HighFive::Group fields_;
  std::vector<FieldDescriptor> field_descriptors_;
  std::optional<std::size_t> particle_count_;
  bool committed_ = false;

}; // class FrameWriterState

auto RunWriterState::begin(std::uint64_t step, double time)
    -> std::shared_ptr<FrameWriterState> {
  TIT_ENSURE(!frame_active_, "Another frame is already active.");
  TIT_ENSURE(std::isfinite(time), "Frame time must be finite.");
  if (last_frame_.has_value()) {
    TIT_ENSURE(step > last_frame_->step(),
               "Frame step must increase monotonically.");
    TIT_ENSURE(time > last_frame_->time(),
               "Frame time must increase monotonically.");
  }

  const auto filename = std::format("frame-{:08}.h5", step);
  const auto final = path_ / "frames" / filename;
  TIT_ENSURE(!std::filesystem::exists(final),
             "Frame '{}' already exists.",
             final.string());
  frame_active_ = true;
  return std::make_shared<FrameWriterState>(shared_from_this(),
                                            FrameDescriptor{step, time},
                                            partial_path(final),
                                            final);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class RunReaderState final {
public:

  explicit RunReaderState(std::filesystem::path path)
      : path_{std::move(path)}, metadata_{read_metadata(path_)} {
    refresh();
  }

  void refresh() {
    const auto index = read_json(path_ / "index.json");
    TIT_ENSURE(index.at("version") == run_format_version,
               "Unsupported run-index version.");
    std::vector<std::pair<FrameDescriptor, std::filesystem::path>> frames;
    for (const auto& item : index.at("frames")) {
      const std::filesystem::path relative = item.at("file").get<std::string>();
      TIT_ENSURE(!relative.is_absolute() &&
                     std::ranges::none_of(relative,
                                          [](const auto& component) {
                                            return component == "..";
                                          }),
                 "Invalid frame path in run index.");
      frames.emplace_back(FrameDescriptor{item.at("step").get<std::uint64_t>(),
                                          item.at("time").get<double>()},
                          path_ / relative);
    }
    frames_ = std::move(frames);
  }

  auto metadata() const -> const RunMetadata& {
    return metadata_;
  }

  auto frames() const noexcept
      -> const std::vector<std::pair<FrameDescriptor, std::filesystem::path>>& {
    return frames_;
  }

  auto path() const noexcept -> const std::filesystem::path& {
    return path_;
  }

private:

  std::filesystem::path path_;
  RunMetadata metadata_;
  std::vector<std::pair<FrameDescriptor, std::filesystem::path>> frames_;

}; // class RunReaderState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace impl

FrameWriter::FrameWriter(std::shared_ptr<impl::FrameWriterState> state) noexcept
    : state_{std::move(state)} {}

FrameWriter::~FrameWriter() = default;

void FrameWriter::write_(std::string_view name,
                         Type type,
                         std::span<const std::byte> data,
                         std::size_t size) {
  TIT_ENSURE(state_ != nullptr, "Frame writer is null.");
  state_->write(name, type, data, size);
}

void FrameWriter::commit() {
  TIT_ENSURE(state_ != nullptr, "Frame writer is null.");
  state_->commit();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RunWriter::RunWriter(std::filesystem::path path, RunMetadata metadata)
    : state_{std::make_shared<impl::RunWriterState>(std::move(path),
                                                    std::move(metadata))} {}

auto RunWriter::begin_frame(std::uint64_t step, double time) -> FrameWriter {
  return FrameWriter{state_->begin(step, time)};
}

auto RunWriter::path() const -> const std::filesystem::path& {
  return state_->path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FrameReader::FrameReader(std::filesystem::path path, FrameDescriptor descriptor)
    : path_{std::move(path)}, descriptor_{descriptor} {}

auto FrameReader::descriptor() const noexcept -> const FrameDescriptor& {
  return descriptor_;
}

auto FrameReader::fields() const -> std::vector<FieldDescriptor> {
  const HighFive::File file{path_.string(), HighFive::File::ReadOnly};
  const auto group = file.getGroup("fields");
  auto names = group.listObjectNames();
  std::ranges::sort(names);
  std::vector<FieldDescriptor> fields;
  fields.reserve(names.size());
  for (auto& name : names) {
    const auto dataset = group.getDataSet(name);
    fields.push_back(read_descriptor(dataset, std::move(name)));
  }
  return fields;
}

auto FrameReader::read_field(std::string_view name) const -> FieldData {
  const HighFive::File file{path_.string(), HighFive::File::ReadOnly};
  const auto group = file.getGroup("fields");
  TIT_ENSURE(group.exist(std::string{name}), "Unknown frame field '{}'.", name);
  const auto dataset = group.getDataSet(std::string{name});
  auto descriptor = read_descriptor(dataset, std::string{name});
  std::vector<std::byte> data(descriptor.size() * descriptor.type().width());
  read_dataset(dataset, descriptor.type(), data);
  return FieldData{std::move(descriptor), std::move(data)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RunReader::RunReader(std::filesystem::path path)
    : state_{std::make_shared<impl::RunReaderState>(std::move(path))} {}

void RunReader::refresh() {
  state_->refresh();
}

auto RunReader::metadata() const -> const RunMetadata& {
  return state_->metadata();
}

auto RunReader::num_frames() const -> std::size_t {
  return state_->frames().size();
}

auto RunReader::frame(std::size_t index) const -> FrameReader {
  const auto& frames = state_->frames();
  TIT_ENSURE(index < frames.size(),
             "Frame index '{}' is out of bounds.",
             index);
  const auto& [descriptor, path] = frames[index];
  TIT_ENSURE(std::filesystem::is_regular_file(path),
             "Committed frame '{}' is missing.",
             path.string());
  return FrameReader{path, descriptor};
}

void RunReader::copy_to(const std::filesystem::path& destination) const {
  TIT_ENSURE(!std::filesystem::exists(destination),
             "Run export destination '{}' already exists.",
             destination.string());
  const auto partial = partial_path(destination);
  TIT_ENSURE(!std::filesystem::exists(partial),
             "Partial run export '{}' already exists.",
             partial.string());

  std::filesystem::create_directories(partial / "frames");
  std::filesystem::create_directories(partial / "checkpoints");
  PartialDirectoryGuard guard{partial};

  const auto source_manifest = path() / "manifest.json";
  TIT_ENSURE(std::filesystem::is_regular_file(source_manifest),
             "Run manifest '{}' is missing.",
             source_manifest.string());
  std::filesystem::copy_file(source_manifest, partial / "manifest.json");

  JSON index = {
      {"version", run_format_version},
      {"frames", JSON::array()},
      {"checkpoints", JSON::array()},
  };
  for (const auto& [descriptor, source] : state_->frames()) {
    TIT_ENSURE(std::filesystem::is_regular_file(source),
               "Committed frame '{}' is missing.",
               source.string());
    const auto relative = std::filesystem::path{"frames"} / source.filename();
    std::filesystem::copy_file(source, partial / relative);
    index["frames"].push_back({
        {"step", descriptor.step()},
        {"time", descriptor.time()},
        {"file", relative.generic_string()},
    });
  }
  write_json_atomic(partial / "index.json", index);

  std::error_code error;
  std::filesystem::rename(partial, destination, error);
  TIT_ENSURE(!error,
             "Unable to publish run export '{}': {}.",
             destination.string(),
             error.message());
  guard.release();
}

auto RunReader::path() const -> const std::filesystem::path& {
  return state_->path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io
