/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-qual"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>    // IWYU pragma: keep
#include <nanobind/stl/string.h>      // IWYU pragma: keep
#include <nanobind/stl/string_view.h> // IWYU pragma: keep
#include <nanobind/stl/vector.h>      // IWYU pragma: keep

#include "tit/core/basic_types.hpp" // IWYU pragma: keep

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

namespace nb = nanobind;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NB_MODULE(_data, m) { // NOLINT
  using namespace tit;
  using Storage = tit::data::DataStorage;
  using SeriesView = tit::data::DataSeriesView<Storage>;
  using TimeStepView = tit::data::DataTimeStepView<Storage>;
  using DataSetView = tit::data::DataSetView<Storage>;
  using DataArray = tit::data::DataArrayView<Storage>;
  using DataType = tit::data::DataType;

  nb::enum_<DataType::Kind>(m, "DataKind")
      .value("UNKNOWN", DataType::Kind::unknown)
      .value("INT_8", DataType::Kind::int8)
      .value("UINT_8", DataType::Kind::uint8)
      .value("INT_16", DataType::Kind::int16)
      .value("UINT_16", DataType::Kind::uint16)
      .value("INT_32", DataType::Kind::int32)
      .value("UINT_32", DataType::Kind::uint32)
      .value("INT_64", DataType::Kind::int64)
      .value("UINT_64", DataType::Kind::uint64)
      .value("FLOAT_32", DataType::Kind::float32)
      .value("FLOAT_64", DataType::Kind::float64);

  nb::enum_<DataType::Rank>(m, "DataRank")
      .value("SCALAR", DataType::Rank::scalar)
      .value("VECTOR", DataType::Rank::vector)
      .value("MATRIX", DataType::Rank::matrix);

  nb::class_<DataType>(m, "DataType")
      .def(nb::init<DataType::Kind, DataType::Rank, uint8_t>())
      .def("__eq__",
           [](DataType& self, nb::object& other) {
             return nb::isinstance<DataType>(other) &&
                    self == nb::cast<DataType>(other);
           })
      .def("__hash__", [](DataType& self) { return self.id(); })
      .def("__repr__",
           [](DataType& self) -> std::string {
             const auto* const kind_name = [kind = self.kind()] {
               switch (kind) {
                 case DataType::Kind::int8:    return "int8";
                 case DataType::Kind::uint8:   return "uint8";
                 case DataType::Kind::int16:   return "int16";
                 case DataType::Kind::uint16:  return "uint16";
                 case DataType::Kind::int32:   return "int32";
                 case DataType::Kind::uint32:  return "uint32";
                 case DataType::Kind::int64:   return "int64";
                 case DataType::Kind::uint64:  return "uint64";
                 case DataType::Kind::float32: return "float32";
                 case DataType::Kind::float64: return "float64";
                 default:                      return "unknown";
               }
             }();
             switch (self.rank()) {
               case DataType::Rank::vector:
                 return std::format("Vec<{}, {}>", kind_name, self.dim());
               case DataType::Rank::matrix:
                 return std::format("Mat<{}, {}>", kind_name, self.dim());
               default: return std::string{kind_name};
             }
           })
      .def_prop_ro("kind", &DataType::kind)
      .def_prop_ro("rank", &DataType::rank)
      .def_prop_ro("dim", &DataType::dim);

  nb::class_<DataArray>(m, "DataArray")
      .def("__eq__",
           [](DataArray& self, nb::object& other_obj) {
             if (!nb::isinstance<DataArray>(other_obj)) return false;
             const auto other = nb::cast<DataArray>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](DataArray& self) { return self.id().get(); })
      .def("__repr__",
           [](DataArray& self) {
             return std::format("tit.data.DataArray('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("type", &DataArray::type)
      .def_prop_ro(
          "data",
          [](DataArray& self) { //
            using Bytes = std::vector<byte_t>;
            auto data = std::make_unique<Bytes>(self.data());
            nb::capsule owner{
                data.get(),
                [](void* data_void_ptr) noexcept {
                  // NOLINTNEXTLINE(*-owning-memory)
                  delete static_cast<Bytes*>(data_void_ptr);
                },
            };
            const auto type = self.type();
            const auto rank = type.rank();
            const auto dim = type.dim();
            const auto sdim = static_cast<int64_t>(dim);
            const auto dtype = nb::dtype<float64_t>();
            auto size = data->size() / sizeof(float64_t);
            for (size_t i = 0; i < static_cast<size_t>(rank); ++i) size /= dim;
            std::array<size_t, 3> shape{};
            std::array<int64_t, 3> strides{}; // in elements
            switch (self.type().rank()) {
              case DataType::Rank::scalar:
                shape = {size};
                strides = {1};
                break;
              case DataType::Rank::vector:
                shape = {size, dim};
                strides = {sdim, 1};
                break;
              case DataType::Rank::matrix:
                shape = {size, dim, dim};
                strides = {sdim * sdim, sdim, 1};
                break;
              default: std::unreachable();
            }
            auto ndarray = nb::ndarray<nb::numpy>{
                /*data=*/data->data(),
                /*ndim=*/static_cast<size_t>(rank) + 1,
                /*shape=*/shape.data(),
                /*owner=*/std::move(owner),
                /*strides=*/strides.data(),
                /*dtype=*/dtype,
            };
            static_cast<void>(data.release()); // NOLINT
            return ndarray;
          },
          nb::rv_policy::automatic);

  nb::class_<DataSetView>(m, "DataSetView")
      .def("__eq__",
           [](DataSetView& self, nb::object& other_obj) {
             if (!nb::isinstance<DataSetView>(other_obj)) return false;
             const auto other = nb::cast<DataSetView>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](DataSetView& self) { return self.id().get(); })
      .def("__repr__",
           [](DataSetView& self) {
             return std::format("tit.data.DataSetView('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("num_arrays", &DataSetView::num_arrays)
      .def_prop_ro("arrays", &DataSetView::arrays)
      .def("find_array", &DataSetView::find_array);

  nb::class_<TimeStepView>(m, "TimeStepView")
      .def("__eq__",
           [](TimeStepView& self, nb::object& other_obj) {
             if (!nb::isinstance<TimeStepView>(other_obj)) return false;
             const auto other = nb::cast<TimeStepView>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](TimeStepView& self) { return self.id().get(); })
      .def("__repr__",
           [](TimeStepView& self) {
             return std::format("tit.data.TimeStepView('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("time", &TimeStepView::time)
      .def_prop_ro("uniforms", &TimeStepView::uniforms)
      .def_prop_ro("varyings", &TimeStepView::varyings);

  nb::class_<SeriesView>(m, "SeriesView")
      .def("__eq__",
           [](SeriesView& self, nb::object& other_obj) {
             if (!nb::isinstance<SeriesView>(other_obj)) return false;
             const auto other = nb::cast<SeriesView>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](SeriesView& self) { return self.id().get(); })
      .def("__repr__",
           [](SeriesView& self) {
             return std::format("tit.data.SeriesView('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("parameters", &SeriesView::parameters)
      .def_prop_ro("num_time_steps", &SeriesView::num_time_steps)
      .def_prop_ro("last_time_step", /*&SeriesView::last_time_step*/
                   [](SeriesView& self) { return self.time_steps().front(); });

  nb::class_<Storage>(m, "Storage")
      .def(nb::init<const std::string&>())
      .def("__hash__",
           [](Storage& self) {
             return std::hash<std::string>{}(self.path().native());
           })
      .def("__repr__",
           [](Storage& self) {
             return std::format("tit.data.Storage('{}')", self.path().native());
           })
      .def_prop_ro("path", [](Storage& self) { return self.path().native(); })
      .def_prop_rw("max_series", &Storage::max_series, &Storage::set_max_series)
      .def_prop_ro("num_series", &Storage::num_series)
      .def_prop_ro("last_series",
                   [](Storage& self) { return self.series().back(); });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma GCC diagnostic pop
