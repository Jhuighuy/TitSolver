/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <highfive/H5DataSpace.hpp>
#include <highfive/H5File.hpp>
#include <tinyxml2.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class HDF5File {
public:

  explicit HDF5File(const std::filesystem::path& path)
      : file_{path.string(), HighFive::File::Overwrite} {
    xdmf_.InsertEndChild(xdmf_.NewDeclaration());

    auto* root = xdmf_.NewElement("Xdmf");
    xdmf_.InsertEndChild(root);
    root->SetAttribute("Version", "3.0");

    domain_ = xdmf_.NewElement("Domain"); // NOLINT
    root->InsertEndChild(domain_);

    grid_collection_ = xdmf_.NewElement("Grid"); // NOLINT
    grid_collection_->SetAttribute("Name", "TimeSeries");
    grid_collection_->SetAttribute("GridType", "Collection");
    grid_collection_->SetAttribute("CollectionType", "Temporal");

    domain_->InsertEndChild(grid_collection_);
  }

  void add(float64_t time, auto& array) {
    const auto& file_str = file_.getName();
    const auto step_name = std::format("Step{:06}", index);
    auto group = file_.createGroup(step_name);

    auto* grid = grid_collection_->InsertNewChildElement("Grid");
    grid->SetAttribute("Name", step_name.c_str());
    grid->SetAttribute("GridType", "Uniform");

    auto* time_el = grid->InsertNewChildElement("Time");
    time_el->SetAttribute("Value", time);

    auto* topology = grid->InsertNewChildElement("Topology");
    topology->SetAttribute("TopologyType", "Polyvertex");
    topology->SetAttribute("NumberOfElements", 16148);

    auto* geometry = grid->InsertNewChildElement("Geometry");
    geometry->SetAttribute("GeometryType", "XY");

    auto* dataitem = geometry->InsertNewChildElement("DataItem");
    dataitem->SetAttribute("Dimensions", "16148 2");
    dataitem->SetAttribute("NumberType", "Float");
    dataitem->SetAttribute("Precision", "8");
    dataitem->SetAttribute("Format", "HDF");
    dataitem->SetText(std::format("{}:/{}/r", file_str, step_name).c_str());

    decltype(auto{array})::varying_fields.for_each([&](auto field) {
      if (!write_to_group(group, field.field_name, array[field])) return;

      auto* attribute = grid->InsertNewChildElement("Attribute");
      attribute->SetAttribute("Name", std::string{field.field_name}.c_str());
      attribute->SetAttribute("AttributeType", "Scalar");
      attribute->SetAttribute("Center", "Node");

      dataitem = attribute->InsertNewChildElement("DataItem");
      dataitem->SetAttribute("Dimensions", "16148");
      dataitem->SetAttribute("NumberType", "Float");
      dataitem->SetAttribute("Precision", "8");
      dataitem->SetAttribute("Format", "HDF");
      dataitem->SetText(
          std::format("{}:/{}/{}", file_str, step_name, field.field_name)
              .c_str());
    });

    xdmf_.SaveFile(
        (std::filesystem::path{file_str}.replace_extension(".xdmf")).c_str());

    ++index;
  }

  template<class Num>
    requires std::is_arithmetic_v<Num>
  auto write_to_group(auto& group, std::string_view name, std::span<Num> data) {
    const HighFive::DataSpace space{{data.size()}};
    auto dataset = group.template createDataSet<Num>(std::string{name}, space);

    const std::vector<Num> flat{data.begin(), data.end()};
    dataset.write(flat);

    return true;
  }

  template<class Num, size_t Dim>
    requires std::is_arithmetic_v<Num>
  auto write_to_group(auto& group,
                      std::string_view name,
                      std::span<Vec<Num, Dim>> data) {
    const HighFive::DataSpace space{std::array{data.size(), Dim}};
    auto dataset = group.template createDataSet<Num>(std::string{name}, space);

    std::vector<std::array<Num, Dim>> flat;
    for (const auto& vec : data) flat.push_back(vec.elems());
    dataset.write(flat);

    return false;
  }

  template<class Num, size_t Dim>
    requires std::is_arithmetic_v<Num>
  auto write_to_group(auto& /*group*/,
                      std::string_view /*name*/,
                      std::span<Mat<Num, Dim>> /*data*/) {
    return false;
  }

  template<class Num>
  auto write_to_group(auto& /*group*/,
                      std::string_view /*name*/,
                      std::span<Num> /*data*/) {
    return false;
  }

private:

  HighFive::File file_;
  tinyxml2::XMLDocument xdmf_;
  tinyxml2::XMLElement* domain_ = nullptr;
  tinyxml2::XMLElement* grid_collection_ = nullptr;
  size_t index = 0;

}; // class HDF5File

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
