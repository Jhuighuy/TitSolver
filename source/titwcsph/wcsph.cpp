#include <algorithm>
#include <cstring>
#include <numbers>

#include "tit/core/basic_types.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/io.hpp"
#include "tit/core/log.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/opencl.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/time.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/partition.hpp"
#include "tit/geom/search.hpp"

#include "tit/data/storage.hpp"

#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/motion_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/time_integrator.hpp"
#include "tit/sph/viscosity.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real>
auto sph_main(CmdArgs /*args*/) -> int {
  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H;
  constexpr Real POOL_HEIGHT = 2.5 * H;

  constexpr Real dr = H / 80.0;

  constexpr auto N_FIXED = 4;
  constexpr auto WATER_M = int(round(L / dr));
  constexpr auto WATER_N = int(round(H / dr));
  constexpr auto POOL_M = int(round(POOL_WIDTH / dr));
  constexpr auto POOL_N = int(round(POOL_HEIGHT / dr));

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2);

  constexpr Real R = 0.2;
  constexpr Real Ma = 0.1;
  constexpr Real CFL = 0.8;
  constexpr Real dt = std::min(CFL * h_0 / cs_0, Real{0.25} * sqrt(h_0 / g));

  // Parameters for the heat equation. Unused for now.
  [[maybe_unused]] constexpr Real kappa_0 = 0.6;
  [[maybe_unused]] constexpr Real c_v = 4184.0;

  // Setup the SPH equations.
  const FluidEquations equations{
      // Standard motion equation.
      MotionEquation{
          // Enabled particle shifting technique.
          ParticleShiftingTechnique{R, Ma, CFL},
      },
      // Continuity equation with no source terms.
      ContinuityEquation{},
      // Momentum equation with gravity source term.
      MomentumEquation{
          // Inviscid flow.
          NoViscosity{},
          // δ-SPH artificial viscosity formulation.
          DeltaSPHArtificialViscosity{cs_0, rho_0},
          // Gravity source term.
          GravitySource{g},
      },
      // No energy equation.
      NoEnergyEquation{},
      // Weakly compressible equation of state.
      LinearTaitEquationOfState{cs_0, rho_0},
      // C2 Wendland's spline kernel.
      QuarticWendlandKernel{},
  };

  // Setup the time integrator.
  RungeKuttaIntegrator time_integrator{equations};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Set of fields is inferred from the equations.
      time_integrator,
  };

  // Generate individual particles.
  size_t num_fixed_particles = 0;
  size_t num_fluid_particles = 0;
  for (auto i = -N_FIXED; i < POOL_M + N_FIXED; ++i) {
    for (auto j = -N_FIXED; j < POOL_N; ++j) {
      const bool is_fixed = (i < 0 || i >= POOL_M) || (j < 0);
      const bool is_fluid = (i < WATER_M) && (j < WATER_N);

      if (is_fixed) num_fixed_particles += 1;
      else if (is_fluid) num_fluid_particles += 1;
      else continue;

      auto a = particles.append(is_fixed ? ParticleType::fixed :
                                           ParticleType::fluid);
      r[a] = dr * Vec{i + Real{0.5}, j + Real{0.5}};
    }
  }
  TIT_INFO("Num. fixed particles: {}", num_fixed_particles);
  TIT_INFO("Num. fluid particles: {}", num_fluid_particles);

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;

  // Density hydrostatic initialization.
  for (const auto a : particles.all()) {
    if (a.has_type(ParticleType::fixed)) {
      rho[a] = rho_0;
      continue;
    }

    // Compute pressure from Poisson problem.
    const auto x = r[a][0];
    const auto y = r[a][1];
    p[a] = rho_0 * g * (H - y);
    for (size_t N = 1; N < 100; N += 2) {
      constexpr auto pi = std::numbers::pi_v<Real>;
      const auto n = static_cast<Real>(N);
      p[a] -= 8 * rho_0 * g * H / pow2(pi) *
              (exp(n * pi * (x - L) / (2 * H)) * cos(n * pi * y / (2 * H))) /
              pow2(n);
    }
    // Recalculate density from EOS.
    rho[a] = rho_0 + p[a] / pow2(cs_0);
  }

  // Setup the particle mesh structure.
  ParticleMesh mesh{
      // Search for the particles using the grid search.
      geom::GridSearch{h_0},
      // Use RIB as the primary partitioning method.
      geom::RecursiveInertialBisection{},
      // Use graph partitioning with larger cell size as the interface
      // partitioning method.
      geom::GridGraphPartition{2 * h_0},
  };

  // Create a data storage to store the particles.  We'll store only one last
  // run result, all the previous runs will be discarded.
  data::DataStorage storage{"./particles.ttdb"};
  storage.set_max_series(1);
  const auto series = storage.create_series();
  particles.write(0.0, series);

  Real time{};
  Stopwatch exectime{};
  Stopwatch printtime{};
  for (size_t n = 0;; ++n) {
    TIT_INFO("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
             n,
             time * sqrt(g / H),
             exectime.cycle(),
             printtime.cycle());
    {
      const StopwatchCycle cycle{exectime};
      time_integrator.step(dt, mesh, particles);
    }
    const auto end = time * sqrt(g / H) >= 6.9;
    if ((n % 100 == 0 && n != 0) || end) {
      const StopwatchCycle cycle{printtime};
      particles.write(time * sqrt(g / H), series);
    }
    if (end) break;
    time += dt;
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct DummyEquations {
  static constexpr meta::Set
      required_fields{r, v, dv_dt, p, rho, drho_dt, grad_rho, parinfo, m, h};
  static constexpr auto modified_fields = required_fields - meta::Set{m, h};
};

void sph_cl_main(CmdArgs /*args*/) {
  constexpr float H = 0.6;   // Water column height.
  constexpr float L = 2 * H; // Water column length.

  constexpr float POOL_WIDTH = 5.366 * H;
  constexpr float POOL_HEIGHT = 2.5 * H;

  constexpr float dr = H / 80.0;

  constexpr auto N_FIXED = 4;
  constexpr auto WATER_M = int(round(L / dr));
  constexpr auto WATER_N = int(round(H / dr));
  constexpr auto POOL_M = int(round(POOL_WIDTH / dr));
  constexpr auto POOL_N = int(round(POOL_HEIGHT / dr));

  constexpr float g = 9.81;
  constexpr float rho_0 = 1000.0;
  constexpr float cs_0 = 20 * sqrt(g * H);
  constexpr float h_0 = 2.0 * dr;
  constexpr float m_0 = rho_0 * pow(dr, 2);

  [[maybe_unused]] constexpr float R = 0.2;
  [[maybe_unused]] constexpr float Ma = 0.1;
  constexpr float CFL = 0.4;
  [[maybe_unused]] constexpr float dt =
      std::min(CFL * h_0 / cs_0, 0.25F * sqrt(h_0 / g));

  // Generate individual particles.
  ParticleArray particles{Space<float, 2>{}, DummyEquations{}};
  size_t num_fixed_particles = 0;
  size_t num_fluid_particles = 0;
  for (auto i = -N_FIXED; i < POOL_M + N_FIXED; ++i) {
    for (auto j = -N_FIXED; j < POOL_N; ++j) {
      const bool is_fixed = (i < 0 || i >= POOL_M) || (j < 0);
      const bool is_fluid = (i < WATER_M) && (j < WATER_N);

      if (is_fixed) num_fixed_particles += 1;
      else if (is_fluid) num_fluid_particles += 1;
      else continue;

      auto a = particles.append(is_fixed ? ParticleType::fixed :
                                           ParticleType::fluid);
      r[a] = dr * Vec{
                      static_cast<float>(i) + 0.5F,
                      static_cast<float>(j) + 0.5F,
                  };
    }
  }
  print("Num. fixed particles: {}", num_fixed_particles);
  print("Num. fluid particles: {}", num_fluid_particles);

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;

  // Density hydrostatic initialization.
  for (const auto a : particles.all()) {
    if (a.has_type(ParticleType::fixed)) {
      rho[a] = rho_0;
      continue;
    }

    // Compute pressure from Poisson problem.
    const auto x = r[a][0];
    const auto y = r[a][1];
    p[a] = rho_0 * g * (H - y);
    for (size_t N = 1; N < 100; N += 2) {
      constexpr auto pi = std::numbers::pi_v<float>;
      const auto n = static_cast<float>(N);
      p[a] -= 8 * rho_0 * g * H / pow2(pi) *
              (exp(n * pi * (x - L) / (2 * H)) * cos(n * pi * y / (2 * H))) /
              pow2(n);
    }
    // Recalculate density from EOS.
    rho[a] = rho_0 + p[a] / pow2(cs_0);
  }

  // Setup the particle mesh structure.
  ParticleMesh mesh{
      // Search for the particles using the grid search.
      geom::GridSearch{h_0},
      // Use RIB as the primary partitioning method.
      geom::RecursiveInertialBisection{},
      // Use graph partitioning with larger cell size as the interface
      // partitioning method.
      geom::GridGraphPartition{2 * h_0},
  };

  // Create a data storage to store the particles.  We'll store only one last
  // run result, all the previous runs will be discarded.
  data::DataStorage storage{"./particles.ttdb"};
  storage.set_max_series(1);
  const auto series = storage.create_series();
  particles.write(0.0, series);

  // Setup OpenCL.
  const auto platform = ocl::Platform::default_();
  const auto device = ocl::Device::default_(platform);
  ocl::Context context{{device}};

  // Create buffers for the particles.
  constexpr auto field_flags =
      ocl::BufferAccess::host_read | ocl::BufferAccess::device_read_write;
  ocl::Mem cl_v{context, field_flags, v[particles]};
  ocl::Mem cl_r{context, field_flags, r[particles]};
  ocl::Mem cl_dv_dt{context, field_flags, dv_dt[particles]};
  ocl::Mem cl_p{context, field_flags, p[particles]};
  ocl::Mem cl_rho{context, field_flags, rho[particles]};
  ocl::Mem cl_drho_dt{context, field_flags, drho_dt[particles]};
  ocl::Mem cl_grad_rho{context, field_flags, grad_rho[particles]};

  constexpr auto adj_flags =
      ocl::BufferAccess::host_write | ocl::BufferAccess::device_read;
  ocl::Mem<size_t> adj_ranges{context,
                              adj_flags,
                              num_fixed_particles + num_fluid_particles + 1};
  ocl::Mem<size_t> adj{context, adj_flags, 1};
  ocl::Mem<size_t> iadj_ranges{context, adj_flags, num_fixed_particles + 1};
  ocl::Mem<size_t> iadj{context, adj_flags, 1};

  const ocl::Program program{context,
                             device,
                             "#include \"fluid_equations.cl\"\n"};

  ocl::Kernel setup_boundary_kernel{program, "setup_boundary"};
  ocl::Kernel compute_density_density_gradient_kernel{
      program,
      "compute_density_density_gradient"};
  ocl::Kernel compute_density_density_time_derivative_kernel{
      program,
      "compute_density_density_time_derivative"};
  ocl::Kernel compute_forces_kernel{program, "compute_forces"};
  ocl::Kernel update_density_kernel{program, "update_density"};
  ocl::Kernel update_velocity_kernel{program, "update_velocity"};

  ocl::CommandQueue queue{context, device};

  const auto setup_boundary = [&] {
    setup_boundary_kernel.set_args(cl_r,
                                   cl_v,
                                   cl_rho,
                                   h_0,
                                   m_0,
                                   iadj_ranges,
                                   iadj,
                                   num_fluid_particles,
                                   num_fluid_particles + num_fixed_particles);
    setup_boundary_kernel.enqueue_exec(queue, {}, {num_fixed_particles});
  };

  const auto compute_density = [&] {
    compute_density_density_gradient_kernel.set_args(cl_r,
                                                     cl_rho,
                                                     cl_grad_rho,
                                                     h_0,
                                                     m_0,
                                                     adj_ranges,
                                                     adj,
                                                     num_fluid_particles);
    compute_density_density_gradient_kernel.enqueue_exec(
        queue,
        {},
        {num_fluid_particles + num_fixed_particles});

    compute_density_density_time_derivative_kernel.set_args(
        cl_r,
        cl_v,
        cl_rho,
        cl_grad_rho,
        cl_drho_dt,
        cl_p,
        cl_dv_dt,
        h_0,
        m_0,
        cs_0,
        rho_0,
        adj_ranges,
        adj,
        num_fluid_particles,
        num_fluid_particles + num_fixed_particles);
    compute_density_density_time_derivative_kernel.enqueue_exec(
        queue,
        {},
        {num_fluid_particles + num_fixed_particles});
  };

  const auto compute_forces = [&] {
    compute_forces_kernel.set_args(cl_r,
                                   cl_rho,
                                   cl_grad_rho,
                                   cl_p,
                                   cl_v,
                                   cl_dv_dt,
                                   h_0,
                                   m_0,
                                   cs_0,
                                   rho_0,
                                   adj_ranges,
                                   adj,
                                   num_fluid_particles);
    compute_forces_kernel.enqueue_exec(queue, {}, {num_fluid_particles});
  };

  const auto update_density = [&] {
    update_density_kernel.set_args(cl_rho, cl_drho_dt, dt, num_fluid_particles);
    update_density_kernel.enqueue_exec(queue, {}, {num_fluid_particles});
  };

  const auto update_velocity = [&] {
    update_velocity_kernel.set_args(cl_r,
                                    cl_v,
                                    cl_dv_dt,
                                    dt,
                                    num_fluid_particles);
    update_velocity_kernel.enqueue_exec(queue, {}, {num_fluid_particles});
  };

  float time = 0.0F;
  for (size_t n = 0;; ++n) {
    print("{:>15}\t\t{:>10.5f}", n, time * sqrt(g / H));

    if (n % 10 == 0) {
      {
        TIT_PROFILE_SECTION("CL::wait_to_search");
        cl_r.enqueue_read(queue, r[particles]);
        queue.finish();
      }

      {
        TIT_PROFILE_SECTION("CL::search");

        mesh.search(particles, [](auto a) {
          constexpr QuarticWendlandKernel W{};
          return W.radius(a);
        });

        adj_ranges.enqueue_write(queue, mesh.adjacency().get_val_ranges());
        if (adj.size() < mesh.adjacency().get_vals().size()) {
          adj = ocl::Mem{context, adj_flags, mesh.adjacency().get_vals()};
        } else {
          adj.enqueue_write(queue, mesh.adjacency().get_vals());
        }

        iadj_ranges.enqueue_write(queue,
                                  mesh.interp_adjacency().get_val_ranges());
        if (iadj.size() < mesh.interp_adjacency().get_vals().size()) {
          iadj =
              ocl::Mem{context, adj_flags, mesh.interp_adjacency().get_vals()};
        } else {
          iadj.enqueue_write(queue, mesh.interp_adjacency().get_vals());
        }
      }
    }

    {
      TIT_PROFILE_SECTION("CL::run_kernels");
      setup_boundary();
      compute_density();
      update_density();
      compute_forces();
      update_velocity();
    }

    const auto end = time * sqrt(g / H) >= 6.5;
    if ((n % 100 == 0 && n != 0) || end) {
      TIT_PROFILE_SECTION("CL::read_and_write");
      cl_r.enqueue_read(queue, r[particles]);
      cl_v.enqueue_read(queue, v[particles]);
      cl_dv_dt.enqueue_read(queue, dv_dt[particles]);
      cl_p.enqueue_read(queue, p[particles]);
      cl_rho.enqueue_read(queue, rho[particles]);
      cl_drho_dt.enqueue_read(queue, drho_dt[particles]);
      cl_grad_rho.enqueue_read(queue, grad_rho[particles]);
      queue.finish();
      particles.write(time * sqrt(g / H), series);
      print("Writing particles to file...");
    }
    if (end) break;
    time += dt;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto x_main(CmdArgs args) -> int {
  // NOLINTNEXTLINE(*-pro-bounds-pointer-arithmetic)
  if (args.argc() >= 2 && std::strcmp(args.argv()[1], "gpu") == 0) {
    sph::sph_cl_main(args);
  } else {
    sph::sph_main<double>(args);
  }
  return 0;
}

} // namespace
} // namespace tit::sph

TIT_IMPLEMENT_MAIN(tit::sph::x_main);
