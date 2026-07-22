/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/logging.hpp"
#include "tit/core/main.hpp"
#include "tit/core/str.hpp"
#include "tit/core/time.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/tessellation.hpp"
#include "tit/geom/winding.hpp"
#include "tit/geom/winding/fast_winding.hpp"
#include "tit/io/parallel_run.hpp"
#include "tit/io/run.hpp"
#include "tit/par/control.hpp"
#include "tit/sph/distributed_particles.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/simulation.hpp"
#include "tit/sph/time_integrator.hpp"

namespace tit::sph::wcsph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct SolverOptions final {
  std::filesystem::path output{"particles.tit-run"};
  std::optional<std::filesystem::path> restart;
  std::optional<std::size_t> max_steps;
  std::size_t snapshot_interval = 100;
  std::size_t checkpoint_interval = 1000;
  std::size_t particles_per_height = 80;
  std::size_t rebalance_interval = 100;
};

auto parse_options(std::span<char*> arguments) -> SolverOptions {
  SolverOptions options;
  const auto parse_size = [](std::string_view option,
                             std::string_view value) -> std::size_t {
    const auto parsed = str_to<std::size_t>(value);
    TIT_ENSURE(parsed.has_value() && *parsed > 0,
               "Option '{}' requires a positive integer, not '{}'.",
               option,
               value);
    return *parsed;
  };

  for (std::size_t index = 1; index < arguments.size(); ++index) {
    const std::string_view option{arguments[index]};
    const auto value = [&]() -> std::string_view {
      TIT_ENSURE(index + 1 < arguments.size(),
                 "Option '{}' requires a value.",
                 option);
      return arguments[++index];
    };
    if (option == "--output") {
      options.output = value();
    } else if (option == "--restart") {
      options.restart = value();
    } else if (option == "--max-steps") {
      options.max_steps = parse_size(option, value());
    } else if (option == "--snapshot-every") {
      options.snapshot_interval = parse_size(option, value());
    } else if (option == "--checkpoint-every") {
      options.checkpoint_interval = parse_size(option, value());
    } else if (option == "--particles-per-height") {
      options.particles_per_height = parse_size(option, value());
    } else if (option == "--rebalance-every") {
      options.rebalance_interval = parse_size(option, value());
    } else {
      TIT_THROW("Unknown solver option '{}'.", option);
    }
  }

  TIT_ENSURE(!options.output.empty(), "Output run path must not be empty.");
  if (options.restart.has_value()) {
    TIT_ENSURE(!options.restart->empty(),
               "Restart run path must not be empty.");
    TIT_ENSURE(
        std::filesystem::absolute(options.output).lexically_normal() !=
            std::filesystem::absolute(*options.restart).lexically_normal(),
        "Restart input and output run paths must differ.");
  }
  return options;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Particles>
void write_snapshot(io::ParallelRunWriter& run,
                    const Particles& particles,
                    std::uint64_t step,
                    double time) {
  const auto count = particles.num_owned();
  const std::vector<ParticleType> kinds(count, ParticleType::fluid);
  auto frame = run.begin_frame(step, time);
  frame.write("id", particles.ids().first(count));
  frame.write("kind", kinds);
  frame.write("r", r[particles].first(count));
  frame.write("v", v[particles].first(count));
  frame.write("rho", rho[particles].first(count));
  frame.commit();
}

template<class Particles>
void write_checkpoint(io::ParallelRunWriter& run,
                      const Particles& particles,
                      std::uint64_t step,
                      double time) {
  const auto count = particles.num_owned();
  const std::vector<ParticleType> kinds(count, ParticleType::fluid);
  const std::vector widths(count, h[particles]);
  auto checkpoint = run.begin_checkpoint(step, time);
  checkpoint.write("id", particles.ids().first(count));
  checkpoint.write("kind", kinds);
  checkpoint.write("r", r[particles].first(count));
  checkpoint.write("v", v[particles].first(count));
  checkpoint.write("rho", rho[particles].first(count));
  checkpoint.write("m", m[particles].first(count));
  checkpoint.write("h", widths);
  checkpoint.commit();
}

template<class Real, class Particles, class Topology>
auto restore_checkpoint(const std::filesystem::path& path,
                        const dist::Communicator& communicator,
                        const Topology& topology,
                        Real expected_width,
                        Particles& particles) -> io::FrameDescriptor {
  const io::RunReader run{path};
  TIT_ENSURE(run.metadata().dimension() == 2,
             "WCSPH restart checkpoint must be two-dimensional.");
  TIT_ENSURE(run.num_checkpoints() > 0,
             "Restart run '{}' has no committed checkpoints.",
             path.string());
  const io::ParallelCheckpointReader checkpoint{
      run.checkpoint(run.num_checkpoints() - 1),
      communicator};
  const auto ids = checkpoint.template read<std::uint64_t>("id");
  const auto kinds = checkpoint.template read<std::uint8_t>("kind");
  const auto positions = checkpoint.template read<Vec<Real, 2>>("r");
  const auto velocities = checkpoint.template read<Vec<Real, 2>>("v");
  const auto densities = checkpoint.template read<Real>("rho");
  const auto masses = checkpoint.template read<Real>("m");
  const auto widths = checkpoint.template read<Real>("h");

  TIT_ENSURE(std::ranges::all_of(kinds,
                                 [](std::uint8_t kind) {
                                   return kind == std::to_underlying(
                                                      ParticleType::fluid);
                                 }),
             "WCSPH checkpoint contains a non-fluid owned particle.");
  const auto local_min = widths.empty() ?
                             std::numeric_limits<Real>::infinity() :
                             std::ranges::min(widths);
  const auto local_max = widths.empty() ?
                             -std::numeric_limits<Real>::infinity() :
                             std::ranges::max(widths);
  const auto global_min = communicator.all_reduce_min(local_min);
  const auto global_max = -communicator.all_reduce_min(-local_max);
  TIT_ENSURE(std::isfinite(global_min) && global_min == global_max,
             "WCSPH checkpoint smoothing width is not uniform.");
  TIT_ENSURE(global_min == expected_width,
             "WCSPH checkpoint smoothing width differs from the active "
             "configuration.");
  h[particles] = global_min;

  particles.reserve(ids.size());
  for (std::size_t index = 0; index < ids.size(); ++index) {
    auto particle =
        particles.append(ParticleType::fluid, ParticleID{ids[index]});
    r[particle] = positions[index];
    v[particle] = velocities[index];
    rho[particle] = densities[index];
    m[particle] = masses[index];
  }
  topology.migrate(particles);
  return checkpoint.descriptor();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real>
auto sph_main(const dist::Communicator& communicator,
              const SolverOptions& options) -> int {
  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H; // Pool width.
  constexpr Real POOL_HEIGHT = 4.0 * H;  // Pool height.

  const Real dr = H / static_cast<Real>(options.particles_per_height);
  const auto water_m = static_cast<std::size_t>(round(L / dr));
  const auto water_n = static_cast<std::size_t>(round(H / dr));

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  const Real h_0 = 2.0 * dr;
  const Real m_0 = rho_0 * pow(dr, 2);
  constexpr Real mu = 0.001;

  // Setup the SPH equations.
  geom::Surface<Vec<Real, 2>> domain;
  domain.append_vert({0.0, POOL_HEIGHT});
  domain.append_vert({POOL_WIDTH, POOL_HEIGHT});
  domain.append_vert({POOL_WIDTH, 0.0});
  domain.append_vert({0.0, 0.0});
  domain.append_face({0, 1});
  domain.append_face({1, 2});
  domain.append_face({2, 3});
  domain.append_face({3, 0});
  domain = geom::tessellate(domain, dr);

  // Another domain for containment tests.
  geom::Surface<Vec<Real, 2>> domain2;
  domain2.append_vert({0.0, 0.0});
  domain2.append_vert({POOL_WIDTH, 0.0});
  domain2.append_vert({POOL_WIDTH, POOL_HEIGHT});
  domain2.append_vert({0.0, POOL_HEIGHT});
  domain2.append_face({0, 1});
  domain2.append_face({1, 2});
  domain2.append_face({2, 3});
  domain2.append_face({3, 0});
  const geom::MakeFastWinding<Real> make_winding;
  const auto containment = make_winding(domain2);

  const SixthOrderWendlandKernel kernel{};
  const SlabParticleTopology topology{communicator,
                                      Real{0},
                                      POOL_WIDTH,
                                      kernel.radius(h_0)};
  Simulation simulation{
      FluidEquations{
          // Constants.
          g,
          mu,
          // Wall boundary.
          domain,
          containment,
          // Weakly compressible equation of state.
          TaitEquationOfState{cs_0, rho_0},
          // C4 Wendland's spline kernel.
          kernel,
      },
      SSPRKIntegrator{SSPRKOrder::three},
      communicator,
      topology,
  };

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Store kernel width uniformly and all other integrator fields per
      // particle.
      ParticleLayout{TypeSet{h},
                     decltype(simulation)::required_fields - TypeSet{h}},
  };

  std::uint64_t completed_step = 0;
  Real time{};
  if (options.restart.has_value()) {
    const auto checkpoint = restore_checkpoint<Real>(*options.restart,
                                                     communicator,
                                                     topology,
                                                     h_0,
                                                     particles);
    completed_step = checkpoint.step();
    time = static_cast<Real>(checkpoint.time());
  } else {
    h[particles] = h_0;
    for (std::size_t i = 0; i < water_m; ++i) {
      for (std::size_t j = 0; j < water_n; ++j) {
        const auto position = dr * Vec{static_cast<Real>(i) + Real{1.0},
                                       static_cast<Real>(j) + Real{1.0}};
        if (topology.owner(position) != communicator.rank()) continue;
        const auto id = ParticleID{static_cast<std::uint64_t>(i) * water_n + j};
        auto particle = particles.append(ParticleType::fluid, id);
        r[particle] = position;
        m[particle] = m_0;
        rho[particle] = rho_0;
      }
    }

    // Density hydrostatic initialization.
    for (const auto particle : particles.owned()) {
      const auto x = r[particle][0];
      const auto y = r[particle][1];
      auto p_a = rho_0 * g * (H - y);
      for (std::size_t k = 1; k < 100; k += 2) {
        constexpr auto pi = std::numbers::pi_v<Real>;
        const auto k_pi = static_cast<Real>(k) * pi;
        p_a -= 8 * rho_0 * g * H / pow2(k_pi) *
               (exp(k_pi * (x - L) / (2 * H)) * cos(k_pi * y / (2 * H)));
      }
      rho[particle] = rho_0 + p_a / pow2(cs_0);
    }
  }

  simulation.rebalance(particles);

  // Replicate immutable fixed boundary particles on every rank.
  const auto first_fixed_id = static_cast<std::uint64_t>(water_m) * water_n;
  for (std::size_t i = 0; i < domain.num_verts(); ++i) {
    auto particle =
        particles.append(ParticleType::fixed, ParticleID{first_fixed_id + i});
    r[particle] = domain.vert(i);
    m[particle] = m_0;
    rho[particle] = rho_0;
  }

  // Setup the particle mesh structure.
  ParticleMesh mesh{
      // Search for the particles using the grid search.
      geom::GridSearch{h_0},
      // Search for the boundary faces using the grid search.
      geom::GridFaceSearch{h_0},
  };

  // Initialize the particles.
  simulation.initialize(mesh, particles);

  // Publish immutable snapshots directly from the owned SoA spans.
  io::ParallelRunWriter run{options.output,
                            io::RunMetadata{"dam-breaking", 2},
                            communicator};
  write_snapshot(run, particles, completed_step, time);

  // Run the simulation.
  Stopwatch exec_time{};
  Stopwatch print_time{};
  std::size_t executed_steps = 0;
  for (auto step = completed_step + 1;; ++step) {
    const auto scaled_time = time * sqrt(g / H);
    if (communicator.rank() == 0) {
      log("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
          step,
          scaled_time,
          exec_time.cycle(),
          print_time.cycle());
    }

    Real dt{};
    {
      const StopwatchCycle cycle{exec_time};
      dt = simulation.step(mesh, particles);
    }
    time += dt;
    ++executed_steps;
    if (step % options.rebalance_interval == 0) {
      simulation.rebalance(particles);
    }

    constexpr auto end_time = Real{10.0};
    const auto next_scaled_time = time * sqrt(g / H);
    const auto physical_end = next_scaled_time >= end_time;
    const auto requested_end =
        options.max_steps.has_value() && executed_steps >= *options.max_steps;
    const auto end = physical_end || requested_end;
    if ((step % options.snapshot_interval == 0) || end) {
      const StopwatchCycle cycle{print_time};
      write_snapshot(run, particles, step, time);
    }
    if ((step % options.checkpoint_interval == 0) || end) {
      const StopwatchCycle cycle{print_time};
      write_checkpoint(run, particles, step, time);
    }

    if (end) break;
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph::wcsph

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  const dist::Environment environment{argc, argv};
  const auto communicator = dist::Communicator::world();
  try {
    const auto options = sph::wcsph::parse_options(
        std::span<char*>{argv, static_cast<std::size_t>(argc)});
    par::init();
    sph::wcsph::sph_main<tit::float64_t>(communicator, options);
  } catch (const std::exception& exception) {
    err("Fatal distributed error on rank {}: {}",
        communicator.rank(),
        exception.what());
    communicator.abort(EXIT_FAILURE);
  } catch (...) {
    err("Unknown fatal distributed error on rank {}.", communicator.rank());
    communicator.abort(EXIT_FAILURE);
  }
});
