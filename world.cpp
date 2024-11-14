#include "types.h"

World::World(std::vector<Particle> particles)
    : particles(particles), grid(particles) {}
