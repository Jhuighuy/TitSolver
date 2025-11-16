/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#include <algorithm>
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
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/type.hpp"
#include "tit/data/export-hdf5.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

namespace tit::data {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// HDF5 writer.
class HDF5Writer final {
public:

  // Create a new HDF5/XDMF3 writer.
  explicit HDF5Writer(const std::filesystem::path& path)
      : file_{path.string(), HighFive::File::Overwrite} {}

  // Write a single frame to the file.
  void write_frame(const std::string& frame_name,
                   DataFrameView<const DataStorage> frame) {
    auto group = file_.createGroup(frame_name);
    for (const auto& array : frame.arrays()) {
      const auto array_name = array.name();
      const auto array_size = array.size();
      const auto array_type = array.type();

      /// @todo Currently, we do not support writing matrices, because
      ///       HDF5 does not support them natively.
      if (array_type.rank() == DataRank::matrix) continue;

      const auto array_data = array.read();

      std::vector space_dims{array_size};
      using enum DataRank;
      switch (array_type.rank()) {
        case scalar: {
          break;
        }

        case vector: {
          space_dims.push_back(array_type.dim());
          break;
        }

        case matrix: {
          const auto dim = array_type.dim();
          space_dims.push_back(dim), space_dims.push_back(dim);
          break;
        }

        default: std::unreachable();
      }
      const HighFive::DataSpace space{space_dims};

      using enum DataKind::ID;
      switch (array_type.kind().id()) {
#define CASE(type)                                                             \
  case type:                                                                   \
    group.createDataSet<type##_t>(array_name, space)                           \
        .write_raw(safe_bit_ptr_cast<type##_t*>(array_data.data()));           \
    break;

        CASE(int8)
        CASE(uint8)
        CASE(int16)
        CASE(uint16)
        CASE(int32)
        CASE(uint32)
        CASE(int64)
        CASE(uint64)
        CASE(float32)
        CASE(float64)

#undef CASE

        default: std::unreachable();
      }
    }
  }

private:

  HighFive::File file_;

}; // class HDF5Writer

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// XDMF3 writer.
class XDMF3Writer final {
public:

  // Create a new HDF5/XDMF3 writer.
  XDMF3Writer() {
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

  // Save the file.
  void save(const std::filesystem::path& path) {
    xdmf_doc_.SaveFile(path.c_str());
  }

  // Write a single frame to the file.
  void write_frame(const std::filesystem::path& hdf5_rel_path,
                   const std::string& frame_name,
                   DataFrameView<const DataStorage> frame) {
    auto* const grid_elem =
        grid_collection_elem_->InsertNewChildElement("Grid");
    grid_elem->SetAttribute("Name", frame_name.c_str());
    grid_elem->SetAttribute("GridType", "Uniform");

    auto* const time_elem = grid_elem->InsertNewChildElement("Time");
    time_elem->SetAttribute("Value", frame.time());

    constexpr std::string_view positions_name = "r";
    const auto positions = frame.find_array(positions_name);
    TIT_ENSURE(positions.has_value(), "Positions array 'r' not found!");
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

    add_data_item_(geometry_elem,
                   hdf5_rel_path,
                   frame_name,
                   positions_name,
                   positions_size,
                   positions_type);

    for (const auto& array : frame.arrays()) {
      const auto array_name = array.name();
      const auto array_size = array.size();
      const auto array_type = array.type();

      /// @todo Currently, we do not support writing matrices, because
      ///       HDF5 does not support them natively.
      if (array_type.rank() == DataRank::matrix) continue;

      auto* const attribute_elem =
          grid_elem->InsertNewChildElement("Attribute");
      attribute_elem->SetAttribute("Name", array_name.c_str());
      attribute_elem->SetAttribute("Center", "Node");

      using enum DataRank;
      switch (array_type.rank()) {
        case scalar:
          attribute_elem->SetAttribute("AttributeType", "Scalar");
          break;

        case vector:
          attribute_elem->SetAttribute("AttributeType", "Vector");
          break;

        case matrix:
          attribute_elem->SetAttribute("AttributeType", "Matrix");
          break;

        default: std::unreachable();
      }

      add_data_item_(attribute_elem,
                     hdf5_rel_path,
                     frame_name,
                     array_name,
                     array_size,
                     array_type);
    }
  }

private:

  static void add_data_item_(tinyxml2::XMLElement* parent_elem,
                             const std::filesystem::path& hdf5_rel_path,
                             std::string_view frame_name,
                             std::string_view array_name,
                             size_t size,
                             DataType type) {
    TIT_ASSERT(parent_elem != nullptr, "Parent element is null!");
    TIT_ASSERT(!frame_name.empty(), "Frame name is empty!");
    TIT_ASSERT(!array_name.empty(), "Array name is empty!");

    auto* const data_item_elem = parent_elem->InsertNewChildElement("DataItem");
    data_item_elem->SetAttribute("Format", "HDF");

    using enum DataRank;
    switch (type.rank()) {
      case scalar: {
        data_item_elem->SetAttribute( //
            "Dimensions",
            std::format("{}", size).c_str());
        break;
      }

      case vector: {
        data_item_elem->SetAttribute(
            "Dimensions",
            std::format("{} {}", size, type.dim()).c_str());
        break;
      }

      case matrix: {
        const auto dim = type.dim();
        data_item_elem->SetAttribute(
            "Dimensions",
            std::format("{} {} {}", size, dim, dim).c_str());
        break;
      }

      default: std::unreachable();
    }

    using enum DataKind::ID;
    switch (type.kind().id()) {
      case int8:
      case uint8:
        data_item_elem->SetAttribute("NumberType", "Int");
        data_item_elem->SetAttribute("Precision", "1");
        break;

      case int16:
      case uint16:
        data_item_elem->SetAttribute("NumberType", "Int");
        data_item_elem->SetAttribute("Precision", "2");
        break;

      case int32:
      case uint32:
        data_item_elem->SetAttribute("NumberType", "Int");
        data_item_elem->SetAttribute("Precision", "4");
        break;

      case int64:
      case uint64:
        data_item_elem->SetAttribute("NumberType", "Int");
        data_item_elem->SetAttribute("Precision", "8");
        break;

      case float32:
        data_item_elem->SetAttribute("NumberType", "Float");
        data_item_elem->SetAttribute("Precision", "4");
        break;

      case float64:
        data_item_elem->SetAttribute("NumberType", "Float");
        data_item_elem->SetAttribute("Precision", "8");
        break;

      default: std::unreachable();
    }

    /// @todo In C++26 there would be no need for `.string()`.
    data_item_elem->SetText(
        std::format("{}:/{}/{}", hdf5_rel_path.string(), frame_name, array_name)
            .c_str());
  }

  tinyxml2::XMLDocument xdmf_doc_;
  tinyxml2::XMLElement* grid_collection_elem_ = nullptr;

}; // class XDMF3Writer

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

void export_hdf5(const std::filesystem::path& path,
                 DataSeriesView<const DataStorage> series) {
  TIT_ENSURE(std::filesystem::exists(path), "Directory does not exist!");
  TIT_ENSURE(std::filesystem::is_directory(path), "Path is not a directory!");

  const auto xdmf_path = path / "particles.xdmf";
  const auto hdf5_path = path / "particles.h5";

  const auto num_frames = series.num_frames();
  const auto padding =
      static_cast<size_t>(std::ceil(std::log10(std::max(1UZ, num_frames))));

  HDF5Writer hdf5_writer{hdf5_path};
  XDMF3Writer xdmf_writer{};

  for (const auto& [index, frame] : std::views::enumerate(series.frames())) {
    const auto name = std::format("frame-{:0{}}", index, padding);
    hdf5_writer.write_frame(name, frame);
    xdmf_writer.write_frame(hdf5_path.filename(), name, frame);
  }

  xdmf_writer.save(xdmf_path);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
