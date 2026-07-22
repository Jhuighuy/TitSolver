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

/// Reserved paths and metadata for one unpublished frame.
class FramePublication final {
public:

  FramePublication(FrameDescriptor descriptor,
                   std::filesystem::path partial,
                   std::filesystem::path final)
      : descriptor_{descriptor}, partial_{std::move(partial)},
        final_{std::move(final)} {}

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

  FrameDescriptor descriptor_;
  std::filesystem::path partial_;
  std::filesystem::path final_;

}; // class FramePublication

/// Create a new run catalog and its initial manifest/index.
auto make_run_publisher(std::filesystem::path path, RunMetadata metadata)
    -> std::shared_ptr<RunPublisherState>;

/// Reserve the next monotonically ordered frame publication.
auto begin_frame_publication(const std::shared_ptr<RunPublisherState>& state,
                             std::uint64_t step,
                             double time) -> FramePublication;

/// Publish a fully closed backend frame and update the committed index.
void publish_frame(const std::shared_ptr<RunPublisherState>& state,
                   const FramePublication& publication,
                   const std::vector<FieldDescriptor>& fields);

/// Release a failed or abandoned frame reservation.
void abandon_frame(const std::shared_ptr<RunPublisherState>& state) noexcept;

/// Run directory path owned by a publisher.
auto publisher_path(const std::shared_ptr<RunPublisherState>& state) noexcept
    -> const std::filesystem::path&;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::io::impl
