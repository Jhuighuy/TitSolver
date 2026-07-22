/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "tit/io/run.hpp"

namespace tit::io::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class RunPublisherState;

/// Kind of immutable object published in a run.
enum class PublicationKind : std::uint8_t {
  frame,
  checkpoint,
};

/// Reserved paths and metadata for one unpublished run object.
class RunPublication final {
public:

  RunPublication(PublicationKind kind,
                 FrameDescriptor descriptor,
                 std::filesystem::path partial,
                 std::filesystem::path final)
      : kind_{kind}, descriptor_{descriptor}, partial_{std::move(partial)},
        final_{std::move(final)} {}

  constexpr auto kind() const noexcept -> PublicationKind {
    return kind_;
  }

  auto descriptor() const noexcept -> const FrameDescriptor& {
    return descriptor_;
  }

  auto partial_path() const noexcept -> const std::filesystem::path& {
    return partial_;
  }

  auto final_path() const noexcept -> const std::filesystem::path& {
    return final_;
  }

private:

  PublicationKind kind_;
  FrameDescriptor descriptor_;
  std::filesystem::path partial_;
  std::filesystem::path final_;

}; // class RunPublication

/// Create a new run catalog and its initial manifest/index.
auto make_run_publisher(std::filesystem::path path, RunMetadata metadata)
    -> std::shared_ptr<RunPublisherState>;

/// Reserve the next monotonically ordered publication of a given kind.
auto begin_publication(const std::shared_ptr<RunPublisherState>& state,
                       PublicationKind kind,
                       std::uint64_t step,
                       double time) -> RunPublication;

/// Publish a fully closed backend object and update the committed index.
void publish(const std::shared_ptr<RunPublisherState>& state,
             const RunPublication& publication,
             const std::vector<FieldDescriptor>& fields);

/// Release a failed or abandoned publication reservation.
void abandon_publication(
    const std::shared_ptr<RunPublisherState>& state) noexcept;

/// Run directory path owned by a publisher.
auto publisher_path(const std::shared_ptr<RunPublisherState>& state) noexcept
    -> const std::filesystem::path&;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io::impl
