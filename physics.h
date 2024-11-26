#ifndef __SPH_PHYSICS
#define __SPH_PHYSICS

#include "types.h"
double compute_density(World *w, Particle *p);
double density_derivative(World *w, Particle *p);
double velocity_divergence(World *w, Particle *p);
vec2 pressure_acceleration(World *w, Particle *pi, double pressure[]);
#endif
