/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "tit/python/nanobind.hpp"
#include "tit/python/nanobind_utils.hpp"

namespace tit {
namespace {

namespace nb = nanobind;

using data::DataType;
using Storage = tit::data::DataStorage;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data_type(nb::module_& m) {
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
      .value("FLOAT_64", DataType::Kind::float64)
      .export_values();

  nb::enum_<DataType::Rank>(m, "DataRank")
      .value("SCALAR", DataType::Rank::scalar)
      .value("VECTOR", DataType::Rank::vector)
      .value("MATRIX", DataType::Rank::matrix)
      .export_values();

  nb::class_<DataType>(m, "DataType")
      .def(nb::init<DataType::Kind, DataType::Rank, uint8_t>())
      .def("__eq__",
           [](const DataType& self, const nb::object& other) {
             return nb::isinstance<DataType>(other) &&
                    self == nb::cast<DataType>(other);
           })
      .def("__hash__", [](const DataType& self) { return self.id(); })
      .def("__repr__",
           [](const DataType& self) -> std::string {
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
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_array_view(nb::module_& m) {
  using DataArray = data::DataArrayView<Storage>;
  nb::class_<DataArray>(m, "DataArray")
      .def("__eq__",
           [](const DataArray& self, const nb::object& other_obj) {
             if (!nb::isinstance<DataArray>(other_obj)) return false;
             const auto other = nb::cast<DataArray>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](const DataArray& self) { return self.id().get(); })
      .def("__repr__",
           [](const DataArray& self) {
             return std::format("tit.data.DataArray('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("type", &DataArray::type)
      .def_prop_ro(
          "data",
          [](const DataArray& self) { //
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
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_dataset_view(nb::module_& m) {
  using DataSet = data::DataSetView<Storage>;
  using DataSetArraysRange = decltype(std::declval<DataSet&>().arrays());
  python::bind_range<DataSetArraysRange>(m, "dataset-arrays-range");
  nb::class_<DataSet>(m, "DataSet")
      .def("__eq__",
           [](DataSet& self, nb::object& other_obj) {
             if (!nb::isinstance<DataSet>(other_obj)) return false;
             const auto other = nb::cast<DataSet>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](DataSet& self) { return self.id().get(); })
      .def("__repr__",
           [](const DataSet& self) {
             return std::format("tit.data.DataSet('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("num_arrays", &DataSet::num_arrays)
      .def_prop_ro("arrays", &DataSet::arrays)
      .def("find_array", &DataSet::find_array);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_time_step_view(nb::module_& m) {
  using TimeStep = data::DataTimeStepView<Storage>;
  nb::class_<TimeStep>(m, "TimeStep")
      .def("__eq__",
           [](const TimeStep& self, const nb::object& other_obj) {
             if (!nb::isinstance<TimeStep>(other_obj)) return false;
             const auto other = nb::cast<TimeStep>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](const TimeStep& self) { return self.id().get(); })
      .def("__repr__",
           [](const TimeStep& self) {
             return std::format("tit.data.TimeStep('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("time", &TimeStep::time)
      .def_prop_ro("uniforms", &TimeStep::uniforms)
      .def_prop_ro("varyings", &TimeStep::varyings);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_series_view(nb::module_& m) {
  using Series = data::DataSeriesView<Storage>;
  using SeriesTimeStepsRange = decltype(std::declval<Series&>().time_steps());
  python::bind_range<SeriesTimeStepsRange>(m, "series-time-steps-range");
  nb::class_<Series>(m, "Series")
      .def("__eq__",
           [](const Series& self, const nb::object& other_obj) {
             if (!nb::isinstance<Series>(other_obj)) return false;
             const auto other = nb::cast<Series>(other_obj);
             return &self.storage() == &other.storage() && self == other;
           })
      .def("__hash__", [](const Series& self) { return self.id().get(); })
      .def("__repr__",
           [](const Series& self) {
             return std::format("tit.data.Series('{}', {})",
                                self.storage().path().native(),
                                self.id().get());
           })
      .def_prop_ro("parameters", &Series::parameters)
      .def_prop_ro("num_time_steps", &Series::num_time_steps)
      .def_prop_ro("time_steps", &Series::time_steps)
      .def_prop_ro("last_time_step", &Series::last_time_step);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data_storage(nb::module_& m) {
  using StorageSeriesRange = decltype(std::declval<Storage&>().series());
  python::bind_range<StorageSeriesRange>(m, "storage-series-range");
  nb::class_<Storage>(m, "Storage")
      .def(nb::init<const std::string&>())
      .def("__hash__",
           [](const Storage& self) {
             return std::hash<std::string>{}(self.path().native());
           })
      .def("__repr__",
           [](const Storage& self) {
             return std::format("tit.data.Storage('{}')", self.path().native());
           })
      .def_prop_ro("path",
                   [](const Storage& self) { return self.path().string(); })
      .def_prop_rw("max_series", &Storage::max_series, &Storage::set_max_series)
      .def_prop_ro("num_series", &Storage::num_series)
      .def_prop_ro("series", [](Storage& self) { return self.series(); })
      .def_prop_ro("last_series",
                   [](Storage& self) { return self.last_series(); });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

NB_MODULE(data, m) { // NOLINT
  tit::bind_data_type(m);
  tit::bind_array_view(m);
  tit::bind_dataset_view(m);
  tit::bind_time_step_view(m);
  tit::bind_series_view(m);
  tit::bind_data_storage(m);
}
