#include "types.h"
#include "kernel.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>
#include "physics.h"
#include "iisph.h"

double iisph_compute_time_step(World *w) {
  double max_vel_sq = 0.0;
  #pragma omp parallel for reduction(max : max_vel_sq)
  for (Particle &p: w->particles) {
    max_vel_sq = std::max(max_vel_sq, norm_square(p.vel));
  }
  double cfl_delta = max_vel_sq == 0.0 ? 1: 0.2 * SUPPORT_RADIUS / sqrt(max_vel_sq);
  double dt = std::min(0.005, cfl_delta);
  w->log("dt", dt);
  return dt;
};

void iisph_compute_pressure(double dt, World *w, double *P) {
  // Use Jacobi iteration to solve a weighted average pressure poission equation
  //   To correct density deviation
  //       ∇²p = (ρ₀ - ρ*) / dt^2
  //   To correct velocity divergence
  //       ∇²p = ρ₀ (∇ . v) / dt
  //  Combining both these:
  //   dt^2 ∇²p = α (ρ₀ - ρ*) + dt ρ ∇ . v
  // or,    A p = s
  // which is a sparse linear system with n variables (p_i) and n equations

  double dt2 = dt * dt;

  // Compute a_ii
  // particles with aii = 0 are excluded from computation
  std::unique_ptr<double[]> aii(new double[w->particles.size()]);
  std::unique_ptr<double[]> s(new double[w->particles.size()]);

#pragma omp parallel
  {
#pragma omp for nowait
    for (Particle& p: w->particles) {
      if (p.boundary_particle) {
        aii[p.idx] = 0;
        continue;
      }

      Particle *pi = &p;
      // Δt²     ∑ⱼ mⱼ  [-∑ₖ mₖ / ρᵢ² ∇W_{ik} + mᵢ / ρᵢ² ∇W_{ji} ] . ∇W_{ij}
      // -Δt²/ρᵢ² ∑ⱼ mⱼ  [ ∑ₖ mₖ      ∇W_{ik} + mᵢ      ∇W_{ij} ] . ∇W_{ij}
      // -Δt²/ρᵢ² ∑ⱼ mⱼ  [ mᵢ ∇W_{ij}    + ∑ₖ mₖ ∇W_{ik}  ] . ∇W_{ij}
      //                                -- (inner sum) --
      //               -------- ( middle term )  --------
      //         (outer sum)

      double outer_sum = 0;

      vec2 inner_sum = {0};
      for (Particle* pk: w->grid->get_neighbours(pi)) {
        inner_sum += pk->mass * gradW(pi->pos, pk->pos);
      }

      for (Particle* pj: w->grid->get_neighbours(pi)) {
        vec2 middle_term = pi->mass * gradW(pi->pos, pj->pos) + inner_sum;
        outer_sum = outer_sum + pj->mass * dot(middle_term, gradW(pi->pos, pj->pos));
      }

      aii[pi->idx] = -dt2 / pow(pi->rho, 2) * outer_sum;
    }

    // Compute s_i
    double alpha = 0.01;
#pragma omp for nowait
    for (Particle& p: w->particles) {
      if (!aii[p.idx]) continue;
      double density_prediction = p.rho + dt * density_derivative(w, &p);
      double density_correction = alpha * std::min(0.0, (w->rho_0 - density_prediction));
      double velocity_correction =  dt * w->rho_0 * velocity_divergence(w, &p);
      s[p.idx] = density_correction + velocity_correction;
    }

    // Initialize P_i = 0
#pragma omp for
    for (int i = 0; i < w->particles.size(); i++) {
      P[i] = 0;
    }
  }

  // Jacobi Iteration to solve
  //     dt² ∇²p = s
  // or,     A p = s
  // P_i <- P_i + Ω/aii (sᵢ - (Ap)ᵢ)
  // until average Ap - s is within tolerance

  // Initialize pressure acceleration
  std::unique_ptr<vec2[]> acc(new vec2[w->particles.size()]);
  int n_fluid = 0;
  #pragma omp parallel for reduction(+: n_fluid)
  for (Particle &p: w->particles) {
    if (!aii[p.idx]) {
      acc[p.idx] = {0};
    } else {
      n_fluid++;
    }
  }
  //printf("n_fluid : %d\n", n_fluid);

  double omega = 0.5; // Relaxed Jacobi iteration
  double error;
  double alpha = 0.01;
  double tolerance = alpha * n_fluid * 0.001 * w->rho_0; // 0.1% of ρ₀
  int iters = 0;
  do {
    error = 0.0;
    iters++;
    // Compute pressure acceleration (i.e. acc = -∇p/ρ)
    #pragma omp parallel for
    for (Particle &p : w->particles) {
      if (!aii[p.idx]) continue;
      acc[p.idx] = pressure_acceleration(w, &p, P);
    }

    #pragma omp parallel for reduction(+: error)
    for (Particle& p: w->particles) {
      if (!aii[p.idx]) continue;
      // Compute (∇²p)ᵢ = -∇(ρ acc)
      //               = ∑ⱼ mⱼ (accᵢ - accⱼ) · ∇W_{ij}
      double laplacian_i = 0.0; // Pressure laplacian
      for (Particle* pj: w->grid->get_neighbours(&p)) {
        laplacian_i += pj->mass * dot(acc[p.idx] - acc[pj->idx], gradW(p.pos, pj->pos));
      }
      // Update P_i <- P_i + Ω/aii (sᵢ - (Ap)ᵢ)
      double s_minus_Ap_i = s[p.idx] - dt2 * laplacian_i;
      assert(aii[p.idx] != 0);
      P[p.idx] = std::max(0.0, P[p.idx] + omega / aii[p.idx] * s_minus_Ap_i);
      assert(!std::isnan(P[p.idx]));
      error += abs(s_minus_Ap_i);
    }

    //printf("[%d] %f >= %f\n", iters, error, tolerance);
  } while (error >= tolerance && iters <= 100);
  w->log("PPE Iters", iters);
  w->log("PPE Error", error);
  w->log("PPE Active", n_fluid);
}

double *IISPH::get_pressure() {
  return pressure;
}

double IISPH::physics_update() {
  // Update neighbours
  w->timer_start("Build Grid");
  w->grid->build();
  w->timer_end("Build Grid");

  // Compute density
  w->timer_start("Compute Density");
  #pragma omp parallel for
  for (Particle& p: w->particles) {
    p.rho = compute_density(w, &p);
  }
  w->timer_end("Compute Density");

  w->timer_start("dt,F_nonp");
  // Compute timestep
  double dt = iisph_compute_time_step(w);

  // Apply non pressure forces
  // rho* Dv/Dt = nu * laplacian(v) + f_ext
  #pragma omp parallel for
  for (Particle& p: w->particles) {
    if (!p.boundary_particle) {
      p.vel += dt * (w->viscous_acceleration(p) + w->external_acceleration(p));
    }
  }
  w->timer_end("dt,F_nonp");

  // Compute pressure forces
  w->timer_start("Compute Pressure");
  iisph_compute_pressure(dt, w, pressure);
  w->timer_end("Compute Pressure");

  w->timer_start("Apply forces");
  // Apply pressure acceleration
  // Dv/Dt = -1/ρ ∇p
  #pragma omp parallel for
  for (Particle& p: w->particles) {
    if (!p.boundary_particle) {
      p.vel += dt * pressure_acceleration(w, &p, pressure);
    }
  }

  // Update position
  #pragma omp parallel for
  for (Particle& p: w->particles) {
    if (!p.boundary_particle) {
      p.pos += dt * p.vel;
    }
  }
  w->timer_end("Apply forces");

  return dt;
}


void IISPH::initialize(World *_w) {
  w = _w;
  pressure = new double[w->particles.size()];
}
