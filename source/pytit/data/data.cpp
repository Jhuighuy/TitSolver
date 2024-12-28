/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/type_traits.hpp"

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "tit/py/bind.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using data::DataType;
using Storage = tit::data::DataStorage;
using DataSet = data::DataSetView<Storage>;
using TimeStep = data::DataTimeStepView<Storage>;
using Series = data::DataSeriesView<Storage>;
using DataArray = data::DataArrayView<Storage>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// String representation of the data type kind.
constexpr auto data_kind_to_string(DataType::Kind kind) -> std::string_view {
  using enum DataType::Kind;
  switch (kind) {
    case int8:    return "int8";
    case uint8:   return "uint8";
    case int16:   return "int16";
    case uint16:  return "uint16";
    case int32:   return "int32";
    case uint32:  return "uint32";
    case int64:   return "int64";
    case uint64:  return "uint64";
    case float32: return "float32";
    case float64: return "float64";
    default:      return "unknown";
  }
}

// String representation of the data type.
constexpr auto data_type_to_string(const DataType& type) -> std::string {
  const auto kind_name = data_kind_to_string(type.kind());
  using enum DataType::Rank;
  switch (type.rank()) {
    case vector: return std::format("Vec<{}, {}>", kind_name, type.dim());
    case matrix: return std::format("Mat<{}, {}>", kind_name, type.dim());
    default:     return std::string{kind_name};
  }
}

void bind_data_type(py::cpp::Module& m) {
#if FALSE
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
#endif

#if FALSE
  nb::enum_<DataType::Rank>(m, "DataRank")
      .value("SCALAR", DataType::Rank::scalar)
      .value("VECTOR", DataType::Rank::vector)
      .value("MATRIX", DataType::Rank::matrix);
#endif

  const auto c = py::cpp::class_<"DataType", DataType>(m);
  c.def_init<py::cpp::Param<DataType::Kind, "kind">,
             py::cpp::Param<DataType::Rank, "rank">,
             py::cpp::Param<uint8_t, "dim">>();
  // .def("__eq__",
  //      [](const DataType& self, const nb::object& other) {
  //        return nb::isinstance<DataType>(other) &&
  //               self == nb::cast<DataType>(other);
  //      })
  c.def<"__hash__", [](const DataType& self) { return self.id(); }>();
  c.def<"__repr__", &data_type_to_string>();
  c.prop<"kind", &DataType::kind>();
  c.prop<"rank", &DataType::rank>();
  c.prop<"rank", [](const DataType& self) {
    return std::to_underlying(self.rank());
  }>();
  c.prop<"dim", &DataType::dim>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_array_view(py::cpp::Module& m) {
  const auto c = py::cpp::class_<"DataArray", DataArray>(m);
  // .def("__eq__",
  //      [](const DataArray& self, const nb::object& other_obj) {
  //        if (!nb::isinstance<DataArray>(other_obj)) return false;
  //        const auto other = nb::cast<DataArray>(other_obj);
  //        return &self.storage() == &other.storage() && self == other;
  //      })
  c.def<"__hash__", [](const DataArray& self) { return self.id().get(); }>();
  c.def<"__repr__", [](const DataArray& self) {
    return std::format("pytit.data.DataArray('{}', {})",
                       self.storage().path().native(),
                       self.id().get());
  }>();
  c.prop<"storage",
         [](DataArray& self) { return py::cpp::find(self.storage()); }>();
  c.prop<"type", &DataArray::type>();
  c.prop<"data",
         [](DataArray& self) { return py::make_memory(self.data()); }>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_dataset_view(py::cpp::Module& m) {
  const auto c = py::cpp::class_<"DataSet", DataSet>(m);
  // .def("__eq__",
  //      [](DataSet& self, nb::object& other_obj) {
  //        if (!nb::isinstance<DataSet>(other_obj)) return false;
  //        const auto other = nb::cast<DataSet>(other_obj);
  //        return &self.storage() == &other.storage() && self == other;
  //      })
  c.def<"__hash__", [](DataSet& self) { return self.id().get(); }>();
  c.def<"__repr__", [](DataSet& self) {
    return std::format("pytit.data.DataSet('{}', {})",
                       self.storage().path().native(),
                       self.id().get());
  }>();
  c.prop<"storage",
         [](DataSet& self) { return py::cpp::find(self.storage()); }>();
  c.prop<"num_arrays", &DataSet::num_arrays>();
  c.prop<"arrays", [](DataSet& self) { return self.arrays(); }>();
  c.def<"find_array",
        &DataSet::find_array,
        py::cpp::Param<std::string_view, "name">>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_time_step_view(py::cpp::Module& m) {
  const auto c = py::cpp::class_<"TimeStep", TimeStep>(m);
  // .def("__eq__",
  //      [](const TimeStep& self, const nb::object& other_obj) {
  //        if (!nb::isinstance<TimeStep>(other_obj)) return false;
  //        const auto other = nb::cast<TimeStep>(other_obj);
  //        return &self.storage() == &other.storage() && self == other;
  //      })
  c.def<"__hash__", [](const TimeStep& self) { return self.id().get(); }>();
  c.def<"__repr__", [](const TimeStep& self) {
    return std::format("pytit.data.TimeStep('{}', {})",
                       self.storage().path().native(),
                       self.id().get());
  }>();
  c.prop<"storage",
         [](TimeStep& self) { return py::cpp::find(self.storage()); }>();
  c.prop<"time", &TimeStep::time>();
  c.prop<"uniforms", [](TimeStep& self) { return self.uniforms(); }>();
  c.prop<"varyings", [](TimeStep& self) { return self.varyings(); }>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_series_view(py::cpp::Module& m) {
  const auto c = py::cpp::class_<"Series", Series>(m);
  // .def("__eq__",
  //      [](const Series& self, const nb::object& other_obj) {
  //        if (!nb::isinstance<Series>(other_obj)) return false;
  //        const auto other = nb::cast<Series>(other_obj);
  //        return &self.storage() == &other.storage() && self == other;
  //      })
  c.def<"__hash__", [](const Series& self) { return self.id().get(); }>();
  c.def<"__repr__", [](const Series& self) {
    return std::format("pytit.data.Series('{}', {})",
                       self.storage().path().native(),
                       self.id().get());
  }>();
  c.prop<"storage",
         [](Series& self) { return py::cpp::find(self.storage()); }>();
  c.prop<"parameters", &Series::parameters>();
  c.prop<"num_time_steps", &Series::num_time_steps>();
  c.prop<"time_steps", [](Series& self) { return self.time_steps(); }>();
  c.prop<"last_time_step",
         [](Series& self) { return self.last_time_step(); }>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data_storage(py::cpp::Module& m) {
  const auto c = py::cpp::class_<"Storage", Storage>(m);
  c.def_init<py::cpp::Param<std::string_view, "path">>();
  c.def<"__hash__", [](const Storage& self) {
    return std::hash<std::string>{}(self.path());
  }>();
  c.def<"__repr__", [](const Storage& self) {
    return std::format("pytit.data.Storage('{}')", self.path().string());
  }>();
  c.prop<"path", [](const Storage& self) { return self.path().string(); }>();
  c.prop<"max_series", &Storage::max_series, &Storage::set_max_series>();
  c.prop<"num_series", &Storage::num_series>();
  c.prop<"series", [](Storage& self) { return self.series(); }>();
  c.prop<"last_series", [](Storage& self) { return self.last_series(); }>();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data(py::cpp::Module& m) {
  bind_data_type(m);
  bind_array_view(m);
  bind_dataset_view(m);
  bind_time_step_view(m);
  bind_series_view(m);
  bind_data_storage(m);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

// Specify the storage as the parent of it's objects.
namespace tit::py::cpp {
template<class Self>
  requires contains_v<Self, DataSet, TimeStep, Series, DataArray>
struct InstanceParent<Self> {
  auto operator()(const Self& self) const -> Object {
    return find(self.storage());
  }
};
} // namespace tit::py::cpp

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_PYCPP_MODULE(data, tit::bind_data)
