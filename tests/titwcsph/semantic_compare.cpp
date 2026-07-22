/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/vec.hpp"
#include "tit/io/run.hpp"

namespace tit::sph::wcsph::testing {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct CanonicalFrame final {
  io::FrameDescriptor descriptor;
  std::vector<std::uint64_t> ids;
  std::vector<Vec<double, 2>> positions;
  std::vector<Vec<double, 2>> velocities;
  std::vector<double> densities;
};

auto canonical_frame(const io::RunReader& run, std::size_t frame_index)
    -> CanonicalFrame {
  const auto frame = run.frame(frame_index);
  auto ids = frame.read<std::uint64_t>("id");
  auto positions = frame.read<Vec<double, 2>>("r");
  auto velocities = frame.read<Vec<double, 2>>("v");
  auto densities = frame.read<double>("rho");
  std::vector<std::size_t> order(ids.size());
  std::ranges::iota(order, std::size_t{0});
  std::ranges::sort(order, {}, [&](std::size_t index) { return ids[index]; });

  const auto reorder = [&]<class Value>(std::vector<Value>& values) {
    TIT_ENSURE(values.size() == order.size(),
               "Frame fields have inconsistent particle counts.");
    std::vector<Value> result;
    result.reserve(values.size());
    for (const auto index : order) result.push_back(values[index]);
    values = std::move(result);
  };
  reorder(ids);
  reorder(positions);
  reorder(velocities);
  reorder(densities);
  return {.descriptor = frame.descriptor(),
          .ids = std::move(ids),
          .positions = std::move(positions),
          .velocities = std::move(velocities),
          .densities = std::move(densities)};
}

auto near(double left,
          double right,
          double relative_tolerance = 1.0e-9,
          double absolute_tolerance = 1.0e-11) -> bool {
  return std::abs(left - right) <=
         absolute_tolerance +
             relative_tolerance * std::max(std::abs(left), std::abs(right));
}

void compare_frames(const CanonicalFrame& reference,
                    const CanonicalFrame& candidate) {
  TIT_ENSURE(reference.descriptor.step() == candidate.descriptor.step(),
             "Compared frames have different completed steps.");
  TIT_ENSURE(near(reference.descriptor.time(), candidate.descriptor.time()),
             "Compared frames have different physical times.");
  TIT_ENSURE(reference.ids == candidate.ids,
             "Compared frames have different stable particle IDs.");
  for (std::size_t index = 0; index < reference.ids.size(); ++index) {
    for (std::size_t component = 0; component < 2; ++component) {
      TIT_ENSURE(near(reference.positions[index][component],
                      candidate.positions[index][component]),
                 "Particle {} position component {} differs across rank "
                 "counts at step {}: {} versus {}.",
                 reference.ids[index],
                 component,
                 reference.descriptor.step(),
                 reference.positions[index][component],
                 candidate.positions[index][component]);
      TIT_ENSURE(near(reference.velocities[index][component],
                      candidate.velocities[index][component]),
                 "Particle {} velocity component {} differs across rank "
                 "counts at step {}: {} versus {}.",
                 reference.ids[index],
                 component,
                 reference.descriptor.step(),
                 reference.velocities[index][component],
                 candidate.velocities[index][component]);
    }
    TIT_ENSURE(near(reference.densities[index], candidate.densities[index]),
               "Particle {} density differs across rank counts at step {}: {} "
               "versus {}.",
               reference.ids[index],
               reference.descriptor.step(),
               reference.densities[index],
               candidate.densities[index]);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void compare_runs(std::span<char*> arguments) {
  TIT_ENSURE(arguments.size() >= 3,
             "Usage: semantic_compare REFERENCE CANDIDATE...");
  const io::RunReader reference_run{arguments[1]};
  TIT_ENSURE(reference_run.num_frames() > 0,
             "Reference run has no committed frames.");
  for (const std::string_view path : arguments.subspan(2)) {
    const io::RunReader candidate_run{path};
    TIT_ENSURE(reference_run.num_frames() == candidate_run.num_frames(),
               "Compared runs have different committed frame counts.");
    for (std::size_t index = 0; index < reference_run.num_frames(); ++index) {
      compare_frames(canonical_frame(reference_run, index),
                     canonical_frame(candidate_run, index));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph::wcsph::testing

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  sph::wcsph::testing::compare_runs(
      std::span<char*>{argv, static_cast<std::size_t>(argc)});
});
