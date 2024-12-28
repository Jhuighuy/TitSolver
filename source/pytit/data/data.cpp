/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TIT_PYTHON_INTERPRETER

#include <array>
#include <format>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type_traits.hpp"

#include "tit/data/storage.hpp"
#include "tit/data/type.hpp"

#include "tit/py/class.hpp"
#include "tit/py/func.hpp"
#include "tit/py/module.hpp"
#include "tit/py/numpy.hpp"
#include "tit/py/object.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using data::DataKind;
using data::DataRank;
using data::DataType;
using Storage = tit::data::DataStorage;
using DataSet = data::DataSetView<Storage>;
using TimeStep = data::DataTimeStepView<Storage>;
using Series = data::DataSeriesView<Storage>;
using DataArray = data::DataArrayView<Storage>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data_type(py::Module& m) {
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

  const py::Class<DataType> c{"DataType", m};
  c.def_init<py::Param<DataKind, "kind">,
             py::Param<DataRank, "rank">,
             py::Param<uint8_t, "dim">>();
  // .def("__eq__",
  //      [](const DataType& self, const nb::object& other) {
  //        return nb::isinstance<DataType>(other) &&
  //               self == nb::cast<DataType>(other);
  //      })
  c.def<"__hash__", [](const DataType& self) { return self.id(); }>();
  c.def<"__repr__", &DataType::name>();
  c.prop<"kind", &DataType::kind>();
  c.prop<"rank", &DataType::rank>();
  c.prop<"rank", [](const DataType& self) {
    return std::to_underlying(self.rank());
  }>();
  c.prop<"dim", &DataType::dim>();
  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_array_view(py::Module& m) {
  const py::Class<DataArray> c{"DataArray", m};
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
  c.prop<"storage", [](DataArray& self) { return py::find(self.storage()); }>();
  c.prop<"type", &DataArray::type>();
  c.prop<"data", [](DataArray& self) { //
    auto data = self.data();
    const auto type = self.type();
    const auto size = data.size() / type.kind().width();
    std::array<size_t, 3> shape{};
    std::span<const size_t> shape_span{shape};
    switch (type.rank()) {
      case DataRank::scalar:
        shape = {size};
        shape_span = shape_span.subspan(0, 1);
        break;
      case DataRank::vector:
        shape = {size / type.dim(), type.dim()};
        shape_span = shape_span.subspan(0, 2);
        break;
      case DataRank::matrix:
        shape = {size / pow2(type.dim()), type.dim(), type.dim()};
        shape_span = shape_span.subspan(0, 3);
        break;
      default: std::unreachable();
    }
    return py::NDArray(type.kind(), std::move(data), shape_span);
  }>();
  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_dataset_view(py::Module& m) {
  const py::Class<DataSet> c{"DataSet", m};
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
  c.prop<"storage", [](DataSet& self) { return py::find(self.storage()); }>();
  c.prop<"num_arrays", &DataSet::num_arrays>();
  c.prop<"arrays", [](DataSet& self) { return self.arrays(); }>();
  c.def<"find_array",
        &DataSet::find_array,
        py::Param<std::string_view, "name">>();
  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_time_step_view(py::Module& m) {
  const py::Class<TimeStep> c{"TimeStep", m};
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
  c.prop<"storage", [](TimeStep& self) { return py::find(self.storage()); }>();
  c.prop<"time", &TimeStep::time>();
  c.prop<"uniforms", [](TimeStep& self) { return self.uniforms(); }>();
  c.prop<"varyings", [](TimeStep& self) { return self.varyings(); }>();
  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_series_view(py::Module& m) {
  const py::Class<Series> c{"Series", m};
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
  c.prop<"storage", [](Series& self) { return py::find(self.storage()); }>();
  c.prop<"parameters", &Series::parameters>();
  c.prop<"num_time_steps", &Series::num_time_steps>();
  c.prop<"time_steps", [](Series& self) { return self.time_steps(); }>();
  c.prop<"last_time_step",
         [](Series& self) { return self.last_time_step(); }>();
  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data_storage(py::Module& m) {
  const py::Class<Storage> c{"Storage", m};
  c.def_init<py::Param<std::string_view, "path">>();
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
  m.add(c);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void bind_data(py::Module& m) {
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
namespace tit::py {
template<class Self>
  requires contains_v<Self, DataSet, TimeStep, Series, DataArray>
struct ObjectParent<Self> {
  auto operator()(const Self& self) const -> Object {
    return find(self.storage());
  }
};
} // namespace tit::py

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_PYTHON_MODULE(data, tit::bind_data)
