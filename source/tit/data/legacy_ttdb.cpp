/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/data/legacy_ttdb.hpp"
#include "tit/data/storage.hpp"
#include "tit/io/run.hpp"
#include "tit/io/type.hpp"

namespace tit::data {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using LegacyArray = ArrayView<const Storage>;
using LegacyFrame = FrameView<const Storage>;

auto sorted_arrays(LegacyFrame frame) -> std::vector<LegacyArray> {
  auto arrays = frame.arrays() | std::ranges::to<std::vector>();
  std::ranges::sort(arrays, {}, [](const auto& array) { return array.name(); });
  return arrays;
}

auto find_array(const std::vector<LegacyArray>& arrays, std::string_view name) {
  return std::ranges::find_if(arrays, [=](const auto& array) {
    return array.name() == name;
  });
}

void validate_array(const LegacyArray& array, io::Type type, std::size_t size) {
  TIT_ENSURE(array.type() == type,
             "Legacy field '{}' has type '{}', not '{}'.",
             array.name(),
             array.type().name(),
             type.name());
  TIT_ENSURE(array.size() == size,
             "Legacy field '{}' has an inconsistent particle count.",
             array.name());
}

void write_array(io::FrameWriter& writer, const LegacyArray& array) {
  writer.write(io::FieldData{
      io::FieldDescriptor{array.name(), array.type(), array.size()},
      array.read()});
}

auto position_array(const std::vector<LegacyArray>& arrays) -> LegacyArray {
  const auto position = find_array(arrays, "r");
  TIT_ENSURE(position != arrays.end(),
             "Legacy frame has no required position field 'r'.");
  const auto type = position->type();
  TIT_ENSURE(type.rank() == io::Rank::vector &&
                 (type.kind().id() == io::Kind::ID::float32 ||
                  type.kind().id() == io::Kind::ID::float64),
             "Legacy position field 'r' is not a floating-point vector.");
  return *position;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

void convert_ttdb(const std::filesystem::path& source,
                  const std::filesystem::path& destination,
                  std::optional<std::size_t> series_index) {
  TIT_ENSURE(source != destination,
             "Legacy source and run destination must be different paths.");
  const Storage storage{source, /*read_only=*/true};
  TIT_ENSURE(storage.num_series() > 0,
             "Legacy storage '{}' contains no series.",
             source.string());
  const SeriesView<const Storage> series = series_index.has_value() ?
                                               storage.series(*series_index) :
                                               storage.last_series();
  TIT_ENSURE(series.num_frames() > 0, "Legacy series contains no frames.");

  const auto first_arrays = sorted_arrays(series.frame(0));
  const auto first_position = position_array(first_arrays);
  const auto initial_size = first_position.size();
  auto name = series.name();
  if (name.empty()) name = "legacy-ttdb";
  io::RunWriter run{
      destination,
      io::RunMetadata{std::move(name), first_position.type().dim()}};

  for (std::size_t frame_index = 0; frame_index < series.num_frames();
       ++frame_index) {
    const auto legacy_frame = series.frame(frame_index);
    const auto arrays = sorted_arrays(legacy_frame);
    const auto position = position_array(arrays);
    const auto particle_count = position.size();
    TIT_ENSURE(position.type() == first_position.type(),
               "Legacy position type changes between frames.");
    TIT_ENSURE(std::ranges::all_of(arrays,
                                   [=](const auto& array) {
                                     return array.size() == particle_count;
                                   }),
               "Legacy frame fields have inconsistent particle counts.");

    auto frame = run.begin_frame(static_cast<std::uint64_t>(frame_index),
                                 legacy_frame.time());
    const auto id = find_array(arrays, "id");
    if (id == arrays.end()) {
      TIT_ENSURE(particle_count == initial_size,
                 "Legacy particle count changes without persisted IDs.");
      std::vector<std::uint64_t> ids(particle_count);
      std::ranges::iota(ids, std::uint64_t{0});
      frame.write("id", ids);
    } else {
      validate_array(*id, io::type_of<std::uint64_t>, particle_count);
      write_array(frame, *id);
    }

    const auto kind = find_array(arrays, "kind");
    if (kind == arrays.end()) {
      frame.write("kind", std::vector<std::uint8_t>(particle_count, 0));
    } else {
      validate_array(*kind, io::type_of<std::uint8_t>, particle_count);
      write_array(frame, *kind);
    }

    for (const auto& array : arrays) {
      if (array.name() == "id" || array.name() == "kind") continue;
      write_array(frame, array);
    }
    frame.commit();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
