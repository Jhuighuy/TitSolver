/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cmath>
#include <filesystem>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <highfive/H5DataSpace.hpp>
#include <highfive/H5File.hpp>
#include <tinyxml2.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/type.hpp"
#include "tit/data/hdf5.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// HDF5/XDMF3 writer.
class HDF5Writer final {
public:

  // Writer is not copyable.
  HDF5Writer(const HDF5Writer&) = delete;
  auto operator=(const HDF5Writer&) -> HDF5Writer& = delete;

  // Writer is not movable.
  HDF5Writer(HDF5Writer&&) = delete;
  auto operator=(HDF5Writer&&) -> HDF5Writer& = delete;

  // Create a new HDF5/XDMF3 writer.
  explicit HDF5Writer(const std::filesystem::path& path)
      : file_{path.string(), HighFive::File::Overwrite} {
    xdmf_doc_.InsertEndChild(xdmf_doc_.NewDeclaration());

    auto* const root_elem = xdmf_doc_.NewElement("Xdmf");
    xdmf_doc_.InsertEndChild(root_elem);
    root_elem->SetAttribute("Version", "3.0");

    auto* const domain_elem = root_elem->InsertNewChildElement("Domain");
    grid_collection_elem_ = domain_elem->InsertNewChildElement("Grid");
    grid_collection_elem_->SetAttribute("Name", "TimeSeries");
    grid_collection_elem_->SetAttribute("GridType", "Collection");
    grid_collection_elem_->SetAttribute("CollectionType", "Temporal");
  }

  // Close the file.
  ~HDF5Writer() noexcept(false) {
    const std::filesystem::path h5_path{file_.getName()};
    const auto xdmf_path = auto{h5_path}.replace_extension(".xdmf");
    xdmf_doc_.SaveFile(xdmf_path.c_str());
  }

  // Write all frames in the series to the file.
  void write_series(DataSeriesView<const DataStorage> series) {
    const auto num_frames = series.num_frames();
    if (num_frames == 0) return;

    const auto padding = static_cast<size_t>(std::ceil(std::log10(num_frames)));
    for (const auto& [index, frame] : std::views::enumerate(series.frames())) {
      const auto name = std::format("Frame{:0{}}", index, padding);
      write_frame(name, frame);
    }
  }

  // Write a single frame to the file.
  void write_frame(const std::string& frame_name,
                   DataFrameView<const DataStorage> frame) {
    auto* const grid_elem =
        grid_collection_elem_->InsertNewChildElement("Grid");
    grid_elem->SetAttribute("Name", frame_name.c_str());
    grid_elem->SetAttribute("GridType", "Uniform");

    auto* const time_elem = grid_elem->InsertNewChildElement("Time");
    time_elem->SetAttribute("Value", frame.time());

    constexpr std::string_view positions_name = "r";
    const auto positions = frame.find_array(positions_name);
    TIT_ENSURE(positions.has_value(), "Positions array not found in frame");
    const auto positions_size = positions->size();
    const auto positions_type = positions->type();

    auto* const topology_elem = grid_elem->InsertNewChildElement("Topology");
    topology_elem->SetAttribute("TopologyType", "Polyvertex");
    topology_elem->SetAttribute("NumberOfElements",
                                static_cast<int>(positions_size));

    auto* const geometry_elem = grid_elem->InsertNewChildElement("Geometry");
    switch (positions_type.dim()) {
      case 1:  geometry_elem->SetAttribute("GeometryType", "X"); break;
      case 2:  geometry_elem->SetAttribute("GeometryType", "XY"); break;
      case 3:  geometry_elem->SetAttribute("GeometryType", "XYZ"); break;
      default: std::unreachable();
    }

    const auto add_data_item = [frame_name,
                                this](tinyxml2::XMLElement* parent_elem,
                                      std::string_view name,
                                      size_t size,
                                      DataType type) {
      auto* const data_item_elem =
          parent_elem->InsertNewChildElement("DataItem");
      data_item_elem->SetAttribute("Format", "HDF");

      const auto dim = type.dim();
      switch (type.rank()) {
        case DataRank::scalar:
          data_item_elem->SetAttribute("Dimensions",
                                       std::format("{}", size).c_str());
          break;
        case DataRank::vector:
          data_item_elem->SetAttribute("Dimensions",
                                       std::format("{} {}", size, dim).c_str());
          break;
        case DataRank::matrix:
          data_item_elem->SetAttribute(
              "Dimensions",
              std::format("{} {} {}", size, dim, dim).c_str());
          break;
        default: std::unreachable();
      }

      switch (type.kind().id()) {
        case DataKind::ID::int8:
        case DataKind::ID::uint8:
          data_item_elem->SetAttribute("NumberType", "Int");
          data_item_elem->SetAttribute("Precision", "1");
          break;
        case DataKind::ID::int16:
        case DataKind::ID::uint16:
          data_item_elem->SetAttribute("NumberType", "Int");
          data_item_elem->SetAttribute("Precision", "2");
          break;
        case DataKind::ID::int32:
        case DataKind::ID::uint32:
          data_item_elem->SetAttribute("NumberType", "Int");
          data_item_elem->SetAttribute("Precision", "4");
          break;
        case DataKind::ID::int64:
        case DataKind::ID::uint64:
          data_item_elem->SetAttribute("NumberType", "Int");
          data_item_elem->SetAttribute("Precision", "8");
          break;
        case DataKind::ID::float32:
          data_item_elem->SetAttribute("NumberType", "Float");
          data_item_elem->SetAttribute("Precision", "4");
          break;
        case DataKind::ID::float64:
          data_item_elem->SetAttribute("NumberType", "Float");
          data_item_elem->SetAttribute("Precision", "8");
          break;
        default: std::unreachable();
      }

      const auto path =
          std::format("{}:/{}/{}", file_.getName(), frame_name, name);
      data_item_elem->SetText(path.c_str());
    };

    add_data_item(geometry_elem,
                  positions_name,
                  positions_size,
                  positions_type);

    auto group = file_.createGroup(frame_name);
    for (const auto& array : frame.arrays()) {
      const auto array_name = array.name();
      const auto array_size = array.size();
      const auto array_type = array.type();
      if (array_type.rank() == DataRank::matrix) continue;

      auto* const attribute_elem =
          grid_elem->InsertNewChildElement("Attribute");
      attribute_elem->SetAttribute("Name", array_name.c_str());
      attribute_elem->SetAttribute("Center", "Node");
      switch (array_type.rank()) {
        case DataRank::scalar:
          attribute_elem->SetAttribute("AttributeType", "Scalar");
          break;
        case DataRank::vector:
          attribute_elem->SetAttribute("AttributeType", "Vector");
          break;
        case DataRank::matrix:
          attribute_elem->SetAttribute("AttributeType", "Matrix");
          break;
        default: std::unreachable();
      }
      add_data_item(attribute_elem, array_name, array_size, array_type);

      std::vector space_dims{array_size};
      switch (array_type.rank()) {
        case DataRank::scalar: break;
        case DataRank::vector: //
          space_dims.push_back(array_type.dim());
          break;
        case DataRank::matrix:
          space_dims.push_back(array_type.dim());
          space_dims.push_back(array_type.dim());
          break;
        default: std::unreachable();
      }
      const HighFive::DataSpace space{space_dims};
      switch (array_type.kind().id()) {
        case DataKind::ID::int8:
          group.createDataSet<int8_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<int8_t*>(array.read().data()));
          break;
        case DataKind::ID::uint8:
          group.createDataSet<uint8_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<uint8_t*>(array.read().data()));
          break;
        case DataKind::ID::int16:
          group.createDataSet<int16_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<int16_t*>(array.read().data()));
          break;
        case DataKind::ID::uint16:
          group.createDataSet<uint16_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<uint16_t*>(array.read().data()));
          break;
        case DataKind::ID::int32:
          group.createDataSet<int32_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<int32_t*>(array.read().data()));
          break;
        case DataKind::ID::uint32:
          group.createDataSet<uint32_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<uint32_t*>(array.read().data()));
          break;
        case DataKind::ID::int64:
          group.createDataSet<int64_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<int64_t*>(array.read().data()));
          break;
        case DataKind::ID::uint64:
          group.createDataSet<uint64_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<uint64_t*>(array.read().data()));
          break;
        case DataKind::ID::float32:
          group.createDataSet<float32_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<float32_t*>(array.read().data()));
          break;
        case DataKind::ID::float64:
          group.createDataSet<float64_t>(array_name, space)
              .write_raw(safe_bit_ptr_cast<float64_t*>(array.read().data()));
          break;
        default: std::unreachable();
      }
    }
  }

private:

  HighFive::File file_;
  tinyxml2::XMLDocument xdmf_doc_;
  tinyxml2::XMLElement* grid_collection_elem_ = nullptr;

}; // class HDF5Writer

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void export_hdf5(const std::filesystem::path& path,
                 DataSeriesView<const DataStorage> series) {
  HDF5Writer writer{path};
  writer.write_series(series);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
