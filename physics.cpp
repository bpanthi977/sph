#include "types.h"
#include "kernel.h"
#include <cmath>
#include <cassert>

double compute_density(World *w, Particle *p) {
  double rho = p->mass * W(0);
  for (Particle *np: w->grid->get_neighbours(p)) {
    rho += np->mass * W(distance(p->pos, np->pos));
  }

  assert(rho >= 0);
  return rho;
}

double density_derivative(World *w, Particle *p) {
  // D rho / Dt = - rho Grad(V)
  //            = - (Grad(rho V) - V Grad(rho))
  //            = - (\sum m_j (v_j - v_i) Grad(W_ij))

  double sum = 0.0;
  for (Particle *np : w->grid->get_neighbours(p)) {
    sum += -np->mass * dot(np->vel - p->vel, gradW(p->pos, np->pos));
  }
  return  sum;
}

double velocity_divergence(World *w, Particle *p) {
  // ∇v = 1/ρ [ ∇(ρv) - v∇ρ ]
  //    = 1/ρᵢ ∑ⱼ mⱼ (vⱼ - vᵢ) ∇W_{ij}

  double sum = 0.0;
  for (Particle *np : w->grid->get_neighbours(p)) {
    sum += np->mass * dot(np->vel - p->vel, gradW(p->pos, np->pos));
  }
  return sum / p->rho;
}

vec2 pressure_acceleration(World *w, Particle *pi, double pressure[]) {
  // ap = Dv/Dt
  //    = - ∇p / ρ
  //    = - ∑ⱼ mⱼ (pᵢ/ρᵢ² + pⱼ/ρⱼ²) ∇W_{ij}
  vec2 sum = {0};
  for (Particle *pj: w->grid->get_neighbours(pi)) {
    if (pj->boundary_particle) {
      // Pressure mirroring by boundary particle
      sum = sum - pj->mass * (pressure[pi->idx] / pow(pi->rho, 2) + pressure[pi->idx] / pow(pj->rho, 2)) * gradW(pi->pos, pj->pos);
    } else {
      sum = sum - pj->mass * (pressure[pi->idx] / pow(pi->rho, 2) + pressure[pj->idx] / pow(pj->rho, 2)) * gradW(pi->pos, pj->pos);
    }
  }
  return sum;
}
