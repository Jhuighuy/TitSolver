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

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/type.hpp"
#include "tit/io/run.hpp"
#include "tit/io/run_backend.hpp"
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

void write_text_atomic(const std::filesystem::path& path,
                       std::string_view text) {
  const auto partial = partial_path(path);
  std::ofstream stream{partial, std::ios::out | std::ios::trunc};
  TIT_ENSURE(stream.is_open(), "Unable to open '{}'.", partial.string());
  stream << text;
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

auto read_fields(const std::filesystem::path& path)
    -> std::vector<FieldDescriptor> {
  const HighFive::File file{path.string(), HighFive::File::ReadOnly};
  std::vector<FieldDescriptor> fields;
  for (const std::string_view group_name : {"particles", "fields"}) {
    if (!file.exist(std::string{group_name})) continue;
    const auto group = file.getGroup(std::string{group_name});
    auto names = group.listObjectNames();
    for (auto& name : names) {
      const auto dataset = group.getDataSet(name);
      fields.push_back(read_descriptor(dataset, std::move(name)));
    }
  }
  std::ranges::sort(fields, {}, &FieldDescriptor::name);
  return fields;
}

auto read_particle_count(const std::filesystem::path& path) -> std::size_t {
  const auto fields = read_fields(path);
  TIT_ENSURE(!fields.empty(),
             "Particle file '{}' has no fields.",
             path.string());
  const auto size = fields.front().size();
  TIT_ENSURE(std::ranges::all_of(
                 fields,
                 [=](const auto& field) { return field.size() == size; }),
             "Particle file '{}' has inconsistent field sizes.",
             path.string());
  return size;
}

auto read_particle_field(const std::filesystem::path& path,
                         std::string_view name) -> FieldData {
  const HighFive::File file{path.string(), HighFive::File::ReadOnly};
  const std::string group_name =
      name == "id" || name == "kind" ? "particles" : "fields";
  TIT_ENSURE(file.exist(group_name), "Unknown particle field '{}'.", name);
  const auto group = file.getGroup(group_name);
  TIT_ENSURE(group.exist(std::string{name}),
             "Unknown particle field '{}'.",
             name);
  const auto dataset = group.getDataSet(std::string{name});
  auto descriptor = read_descriptor(dataset, std::string{name});
  std::vector<std::byte> data(descriptor.size() * descriptor.type().width());
  read_dataset(dataset, descriptor.type(), data);
  return FieldData{std::move(descriptor), std::move(data)};
}

auto xml_escape(std::string_view text) -> std::string {
  std::string escaped;
  escaped.reserve(text.size());
  for (const auto character : text) {
    switch (character) {
      case '&':  escaped += "&amp;"; break;
      case '<':  escaped += "&lt;"; break;
      case '>':  escaped += "&gt;"; break;
      case '"':  escaped += "&quot;"; break;
      case '\'': escaped += "&apos;"; break;
      default:   escaped += character; break;
    }
  }
  return escaped;
}

auto xdmf_number_type(Kind kind) -> std::string_view {
  using enum Kind::ID;
  switch (kind.id()) {
    case int8:    [[fallthrough]];
    case int16:   [[fallthrough]];
    case int32:   [[fallthrough]];
    case int64:   return "Int";
    case uint8:   [[fallthrough]];
    case uint16:  [[fallthrough]];
    case uint32:  [[fallthrough]];
    case uint64:  return "UInt";
    case float32: [[fallthrough]];
    case float64: return "Float";
    default:      std::unreachable();
  }
}

auto xdmf_attribute_type(Rank rank) -> std::string_view {
  using enum Rank;
  switch (rank) {
    case scalar: return "Scalar";
    case vector: return "Vector";
    case matrix: return "Tensor";
    default:     std::unreachable();
  }
}

auto xdmf_geometry_type(std::size_t dimension) -> std::string_view {
  switch (dimension) {
    case 1:  return "X";
    case 2:  return "XY";
    case 3:  return "XYZ";
    default: std::unreachable();
  }
}

auto xdmf_dimensions(const FieldDescriptor& field) -> std::string {
  switch (field.type().rank()) {
    case Rank::scalar: return std::format("{}", field.size());
    case Rank::vector:
      return std::format("{} {}", field.size(), field.type().dim());
    case Rank::matrix:
      return std::format("{} {} {}",
                         field.size(),
                         field.type().dim(),
                         field.type().dim());
    default: std::unreachable();
  }
}

auto xdmf_dataset_path(std::string_view name) -> std::string {
  const std::string_view group =
      name == "id" || name == "kind" ? "particles" : "fields";
  return std::format("/{}/{}", group, name);
}

auto xdmf_data_item(const std::filesystem::path& relative,
                    const FieldDescriptor& field) -> std::string {
  return std::format(
      "<DataItem Dimensions=\"{}\" NumberType=\"{}\" Precision=\"{}\" "
      "Format=\"HDF\">{}:{}</DataItem>",
      xdmf_dimensions(field),
      xdmf_number_type(field.type().kind()),
      field.type().kind().width(),
      xml_escape(relative.generic_string()),
      xml_escape(xdmf_dataset_path(field.name())));
}

auto xdmf_fields(const std::filesystem::path& path,
                 const JSON& manifest,
                 const JSON& item) -> std::vector<FieldDescriptor> {
  if (!item.contains("size")) return read_fields(path);
  const auto size = item.at("size").get<std::uint64_t>();
  TIT_ENSURE(size <= std::numeric_limits<std::size_t>::max(),
             "Frame is too large for this platform.");
  std::vector<FieldDescriptor> fields;
  for (const auto& field : manifest.at("fields")) {
    fields.emplace_back(field.at("name").get<std::string>(),
                        Type{field.at("type").get<std::uint32_t>()},
                        static_cast<std::size_t>(size));
  }
  return fields;
}

void write_xdmf_view(const std::filesystem::path& path,
                     const RunMetadata& metadata,
                     const JSON& manifest,
                     const JSON& index) {
  TIT_ENSURE(index.at("version") == run_format_version,
             "Unsupported run-index version.");

  std::string xdmf = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<Xdmf Version=\"3.0\">\n"
                     "  <Domain>\n";
  xdmf += std::format("    <Grid Name=\"{}\" GridType=\"Collection\" "
                      "CollectionType=\"Temporal\">\n",
                      xml_escape(metadata.name()));

  for (const auto& item : index.at("frames")) {
    const std::filesystem::path relative = item.at("file").get<std::string>();
    TIT_ENSURE(!relative.is_absolute() &&
                   std::ranges::none_of(
                       relative,
                       [](const auto& component) { return component == ".."; }),
               "Invalid frame path in run index.");
    const auto fields = xdmf_fields(path / relative, manifest, item);
    const auto position = std::ranges::find_if(fields, [](const auto& field) {
      return field.name() == "r";
    });
    TIT_ENSURE(position != fields.end(),
               "Committed frame '{}' has no position field.",
               relative.string());
    TIT_ENSURE(std::ranges::all_of(fields,
                                   [&](const auto& field) {
                                     return field.size() == position->size();
                                   }),
               "Committed frame '{}' has inconsistent field sizes.",
               relative.string());

    const auto step = item.at("step").get<std::uint64_t>();
    const auto time = item.at("time").get<double>();
    xdmf += std::format("      <Grid Name=\"step-{}\" GridType=\"Uniform\">\n"
                        "        <Time Value=\"{}\"/>\n"
                        "        <Topology TopologyType=\"Polyvertex\" "
                        "NumberOfElements=\"{}\"/>\n"
                        "        <Geometry GeometryType=\"{}\">\n"
                        "          {}\n"
                        "        </Geometry>\n",
                        step,
                        time,
                        position->size(),
                        xdmf_geometry_type(metadata.dimension()),
                        xdmf_data_item(relative, *position));
    for (const auto& field : fields) {
      if (field.name() == "r") continue;
      xdmf += std::format("        <Attribute Name=\"{}\" AttributeType=\"{}\" "
                          "Center=\"Node\">\n"
                          "          {}\n"
                          "        </Attribute>\n",
                          xml_escape(field.name()),
                          xdmf_attribute_type(field.type().rank()),
                          xdmf_data_item(relative, field));
    }
    xdmf += "      </Grid>\n";
  }
  xdmf += "    </Grid>\n  </Domain>\n</Xdmf>\n";
  write_text_atomic(path / "run.xdmf", xdmf);
}

void regenerate_xdmf_view(const std::filesystem::path& path) {
  write_xdmf_view(path,
                  read_metadata(path),
                  read_json(path / "manifest.json"),
                  read_json(path / "index.json"));
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

class RunPublisherState final {
public:

  RunPublisherState(std::filesystem::path path, RunMetadata metadata)
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
        {"checkpoint_fields", JSON::array()},
    };
    index_ = {
        {"version", run_format_version},
        {"frames", JSON::array()},
        {"checkpoints", JSON::array()},
    };
    write_json_atomic(path_ / "manifest.json", manifest_);
    write_xdmf_view(path_, metadata_, manifest_, index_);
    write_json_atomic(path_ / "index.json", index_);
  }

  auto begin(PublicationKind kind, std::uint64_t step, double time)
      -> RunPublication;

  void publish(const RunPublication& publication,
               const std::vector<FieldDescriptor>& fields) {
    TIT_ENSURE(publication_active_, "No run publication is active.");
    TIT_ENSURE(!fields.empty(), "Cannot publish an empty particle file.");
    const auto find_field = [&](std::string_view name) {
      return std::ranges::find(fields, name, &FieldDescriptor::name);
    };
    const auto id = find_field("id");
    const auto kind = find_field("kind");
    const auto position = find_field("r");
    TIT_ENSURE(id != fields.end() && kind != fields.end() &&
                   position != fields.end(),
               "A particle file must contain 'id', 'kind', and 'r' fields.");
    TIT_ENSURE(id->type() == type_of<std::uint64_t>,
               "Particle field 'id' must have type uint64_t.");
    TIT_ENSURE(kind->type() == type_of<std::uint8_t>,
               "Particle field 'kind' must have type uint8_t.");
    const auto position_type = position->type();
    TIT_ENSURE(position_type.rank() == Rank::vector &&
                   position_type.dim() == metadata_.dimension() &&
                   (position_type.kind().id() == Kind::ID::float32 ||
                    position_type.kind().id() == Kind::ID::float64),
               "Particle field 'r' must be a floating-point vector with run "
               "dimension {}.",
               metadata_.dimension());

    auto& schema = publication.kind() == PublicationKind::frame ?
                       frame_schema_ :
                       checkpoint_schema_;
    const std::string_view schema_name =
        publication.kind() == PublicationKind::frame ? "fields" :
                                                       "checkpoint_fields";
    if (schema.empty()) {
      schema = fields;
      manifest_[schema_name] = schema_json(schema);
      write_json_atomic(path_ / "manifest.json", manifest_);
    } else {
      TIT_ENSURE(same_schema(schema, fields),
                 "Particle file schema differs from the run schema.");
    }

    std::error_code error;
    std::filesystem::rename(publication.partial_path(),
                            publication.final_path(),
                            error);
    TIT_ENSURE(!error,
               "Unable to publish frame '{}': {}.",
               publication.final_path().string(),
               error.message());

    const auto& descriptor = publication.descriptor();
    const std::string_view index_name =
        publication.kind() == PublicationKind::frame ? "frames" : "checkpoints";
    index_[index_name].push_back({
        {"step", descriptor.step()},
        {"time", descriptor.time()},
        {"size", id->size()},
        {"file",
         std::filesystem::relative(publication.final_path(), path_)
             .generic_string()},
    });
    if (publication.kind() == PublicationKind::frame) {
      write_xdmf_view(path_, metadata_, manifest_, index_);
    }
    write_json_atomic(path_ / "index.json", index_);
    auto& last = publication.kind() == PublicationKind::frame ?
                     last_frame_ :
                     last_checkpoint_;
    last = descriptor;
    publication_active_ = false;
  }

  void abandon() noexcept {
    publication_active_ = false;
  }

  auto path() const noexcept -> const std::filesystem::path& {
    return path_;
  }

private:

  std::filesystem::path path_;
  RunMetadata metadata_;
  JSON manifest_;
  JSON index_;
  std::vector<FieldDescriptor> frame_schema_;
  std::vector<FieldDescriptor> checkpoint_schema_;
  std::optional<FrameDescriptor> last_frame_;
  std::optional<FrameDescriptor> last_checkpoint_;
  bool publication_active_ = false;

}; // class RunPublisherState

class ParticleFileWriterState final {
  class PartialFrameGuard final {
  public:

    PartialFrameGuard(std::shared_ptr<RunPublisherState> run,
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

    std::shared_ptr<RunPublisherState> run_;
    std::filesystem::path partial_;
    bool active_ = true;

  }; // class PartialFrameGuard

public:

  ParticleFileWriterState(std::shared_ptr<RunPublisherState> run,
                          RunPublication publication)
      : run_{std::move(run)}, guard_{run_, publication.partial_path()},
        publication_{std::move(publication)},
        file_{std::make_unique<HighFive::File>(
            publication_.partial_path().string(),
            HighFive::File::Overwrite)} {
    file_->createAttribute("format_version", run_format_version);
    file_->createAttribute("publication_kind",
                           std::to_underlying(publication_.kind()));
    file_->createAttribute("step", publication_.descriptor().step());
    file_->createAttribute("time", publication_.descriptor().time());
    particles_ =
        std::make_unique<HighFive::Group>(file_->createGroup("particles"));
    fields_ = std::make_unique<HighFive::Group>(file_->createGroup("fields"));
  }

  void write(std::string_view name,
             Type type,
             std::span<const std::byte> data,
             std::size_t size) {
    TIT_ENSURE(!committed_, "Particle file is already committed.");
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

    const auto& group = name == "id" || name == "kind" ? particles_ : fields_;
    TIT_ENSURE(group != nullptr, "Particle file is already closed.");
    write_dataset(*group, name, type, data, size);
    field_descriptors_.emplace_back(std::string{name}, type, size);
  }

  void commit() {
    TIT_ENSURE(!committed_, "Particle file is already committed.");
    TIT_ENSURE(file_ != nullptr, "Particle file is already closed.");
    file_->flush();
    fields_.reset();
    particles_.reset();
    file_.reset();
    run_->publish(publication_, field_descriptors_);
    guard_.release();
    committed_ = true;
  }

private:

  std::shared_ptr<RunPublisherState> run_;
  PartialFrameGuard guard_;
  RunPublication publication_;
  std::unique_ptr<HighFive::File> file_;
  std::unique_ptr<HighFive::Group> particles_;
  std::unique_ptr<HighFive::Group> fields_;
  std::vector<FieldDescriptor> field_descriptors_;
  std::optional<std::size_t> particle_count_;
  bool committed_ = false;

}; // class ParticleFileWriterState

auto RunPublisherState::begin(PublicationKind kind,
                              std::uint64_t step,
                              double time) -> RunPublication {
  TIT_ENSURE(!publication_active_,
             "Another run publication is already active.");
  TIT_ENSURE(std::isfinite(time), "Publication time must be finite.");
  const auto& last =
      kind == PublicationKind::frame ? last_frame_ : last_checkpoint_;
  if (last.has_value()) {
    TIT_ENSURE(step > last->step(),
               "Publication step must increase monotonically.");
    TIT_ENSURE(time > last->time(),
               "Publication time must increase monotonically.");
  }

  const auto is_frame = kind == PublicationKind::frame;
  const std::string_view stem = is_frame ? "frame" : "checkpoint";
  const std::string_view directory = is_frame ? "frames" : "checkpoints";
  const auto filename = std::format("{}-{:08}.h5", stem, step);
  const auto final = path_ / directory / filename;
  TIT_ENSURE(!std::filesystem::exists(final),
             "Run publication '{}' already exists.",
             final.string());
  publication_active_ = true;
  return RunPublication{kind,
                        FrameDescriptor{step, time},
                        partial_path(final),
                        final};
}

auto make_run_publisher(std::filesystem::path path, RunMetadata metadata)
    -> std::shared_ptr<RunPublisherState> {
  return std::make_shared<RunPublisherState>(std::move(path),
                                             std::move(metadata));
}

auto begin_publication(const std::shared_ptr<RunPublisherState>& state,
                       PublicationKind kind,
                       std::uint64_t step,
                       double time) -> RunPublication {
  TIT_ENSURE(state != nullptr, "Run publisher is null.");
  return state->begin(kind, step, time);
}

void publish(const std::shared_ptr<RunPublisherState>& state,
             const RunPublication& publication,
             const std::vector<FieldDescriptor>& fields) {
  TIT_ENSURE(state != nullptr, "Run publisher is null.");
  state->publish(publication, fields);
}

void abandon_publication(
    const std::shared_ptr<RunPublisherState>& state) noexcept {
  if (state != nullptr) state->abandon();
}

auto publisher_path(const std::shared_ptr<RunPublisherState>& state) noexcept
    -> const std::filesystem::path& {
  TIT_ASSERT(state != nullptr, "Run publisher is null.");
  return state->path();
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
    const auto read_entries = [&](std::string_view name) {
      std::vector<std::pair<FrameDescriptor, std::filesystem::path>> entries;
      for (const auto& item : index.at(name)) {
        const std::filesystem::path relative =
            item.at("file").get<std::string>();
        TIT_ENSURE(!relative.is_absolute() &&
                       std::ranges::none_of(relative,
                                            [](const auto& component) {
                                              return component == "..";
                                            }),
                   "Invalid particle-file path in run index.");
        entries.emplace_back(
            FrameDescriptor{item.at("step").get<std::uint64_t>(),
                            item.at("time").get<double>()},
            path_ / relative);
      }
      return entries;
    };
    frames_ = read_entries("frames");
    checkpoints_ = read_entries("checkpoints");
  }

  auto metadata() const -> const RunMetadata& {
    return metadata_;
  }

  auto frames() const noexcept
      -> const std::vector<std::pair<FrameDescriptor, std::filesystem::path>>& {
    return frames_;
  }

  auto checkpoints() const noexcept
      -> const std::vector<std::pair<FrameDescriptor, std::filesystem::path>>& {
    return checkpoints_;
  }

  auto path() const noexcept -> const std::filesystem::path& {
    return path_;
  }

private:

  std::filesystem::path path_;
  RunMetadata metadata_;
  std::vector<std::pair<FrameDescriptor, std::filesystem::path>> frames_;
  std::vector<std::pair<FrameDescriptor, std::filesystem::path>> checkpoints_;

}; // class RunReaderState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace impl

FrameWriter::FrameWriter(
    std::shared_ptr<impl::ParticleFileWriterState> state) noexcept
    : state_{std::move(state)} {}

FrameWriter::~FrameWriter() = default;

void FrameWriter::write_(std::string_view name,
                         Type type,
                         std::span<const std::byte> data,
                         std::size_t size) {
  TIT_ENSURE(state_ != nullptr, "Frame writer is null.");
  state_->write(name, type, data, size);
}

void FrameWriter::write(const FieldData& field) {
  const auto& descriptor = field.descriptor();
  write_(descriptor.name(), descriptor.type(), field.data(), descriptor.size());
}

void FrameWriter::commit() {
  TIT_ENSURE(state_ != nullptr, "Frame writer is null.");
  state_->commit();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CheckpointWriter::CheckpointWriter(
    std::shared_ptr<impl::ParticleFileWriterState> state) noexcept
    : state_{std::move(state)} {}

CheckpointWriter::~CheckpointWriter() = default;

void CheckpointWriter::write_(std::string_view name,
                              Type type,
                              std::span<const std::byte> data,
                              std::size_t size) {
  TIT_ENSURE(state_ != nullptr, "Checkpoint writer is null.");
  state_->write(name, type, data, size);
}

void CheckpointWriter::write(const FieldData& field) {
  const auto& descriptor = field.descriptor();
  write_(descriptor.name(), descriptor.type(), field.data(), descriptor.size());
}

void CheckpointWriter::commit() {
  TIT_ENSURE(state_ != nullptr, "Checkpoint writer is null.");
  state_->commit();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RunWriter::RunWriter(std::filesystem::path path, RunMetadata metadata)
    : state_{impl::make_run_publisher(std::move(path), std::move(metadata))} {}

auto RunWriter::begin_frame(std::uint64_t step, double time) -> FrameWriter {
  return FrameWriter{std::make_shared<impl::ParticleFileWriterState>(
      state_,
      impl::begin_publication(state_,
                              impl::PublicationKind::frame,
                              step,
                              time))};
}

auto RunWriter::begin_checkpoint(std::uint64_t step, double time)
    -> CheckpointWriter {
  return CheckpointWriter{std::make_shared<impl::ParticleFileWriterState>(
      state_,
      impl::begin_publication(state_,
                              impl::PublicationKind::checkpoint,
                              step,
                              time))};
}

auto RunWriter::path() const -> const std::filesystem::path& {
  return impl::publisher_path(state_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

FrameReader::FrameReader(std::filesystem::path path, FrameDescriptor descriptor)
    : path_{std::move(path)}, descriptor_{descriptor} {}

auto FrameReader::descriptor() const noexcept -> const FrameDescriptor& {
  return descriptor_;
}

auto FrameReader::fields() const -> std::vector<FieldDescriptor> {
  return read_fields(path_);
}

auto FrameReader::read_field(std::string_view name) const -> FieldData {
  return read_particle_field(path_, name);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CheckpointReader::CheckpointReader(std::filesystem::path path,
                                   FrameDescriptor descriptor)
    : path_{std::move(path)}, descriptor_{descriptor} {}

auto CheckpointReader::descriptor() const noexcept -> const FrameDescriptor& {
  return descriptor_;
}

auto CheckpointReader::fields() const -> std::vector<FieldDescriptor> {
  return read_fields(path_);
}

auto CheckpointReader::read_field(std::string_view name) const -> FieldData {
  return read_particle_field(path_, name);
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

auto RunReader::num_checkpoints() const -> std::size_t {
  return state_->checkpoints().size();
}

auto RunReader::checkpoint(std::size_t index) const -> CheckpointReader {
  const auto& checkpoints = state_->checkpoints();
  TIT_ENSURE(index < checkpoints.size(),
             "Checkpoint index '{}' is out of bounds.",
             index);
  const auto& [descriptor, path] = checkpoints[index];
  TIT_ENSURE(std::filesystem::is_regular_file(path),
             "Committed checkpoint '{}' is missing.",
             path.string());
  return CheckpointReader{path, descriptor};
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
        {"size", read_particle_count(source)},
        {"file", relative.generic_string()},
    });
  }
  for (const auto& [descriptor, source] : state_->checkpoints()) {
    TIT_ENSURE(std::filesystem::is_regular_file(source),
               "Committed checkpoint '{}' is missing.",
               source.string());
    const auto relative =
        std::filesystem::path{"checkpoints"} / source.filename();
    std::filesystem::copy_file(source, partial / relative);
    index["checkpoints"].push_back({
        {"step", descriptor.step()},
        {"time", descriptor.time()},
        {"size", read_particle_count(source)},
        {"file", relative.generic_string()},
    });
  }
  write_xdmf_view(partial,
                  read_metadata(partial),
                  read_json(partial / "manifest.json"),
                  index);
  write_json_atomic(partial / "index.json", index);

  std::error_code error;
  std::filesystem::rename(partial, destination, error);
  TIT_ENSURE(!error,
             "Unable to publish run export '{}': {}.",
             destination.string(),
             error.message());
  guard.release();
}

void RunReader::regenerate_xdmf() const {
  regenerate_xdmf_view(path());
}

auto RunReader::path() const -> const std::filesystem::path& {
  return state_->path();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io
