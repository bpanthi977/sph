#include <_types/_uint32_t.h>
#include <cmath>
#include <fstream>
#include <ios>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "vec2.h"
#include "types.h"
#include "kernel.h"

Particle make_particle(double x, double y) {
  Particle p = {0};
  p.pos.x = x;
  p.pos.y = y;
  return p;
}

void compute_density(World *w) {
  for (auto& p: w->particles) {
    p.rho = 0.0;
    NeighbourIterator it = w->grid.get_neighbours(p);
    it.start();
    while (it.has_next()) {
      Particle *np = it.next();
      p.rho += np->mass * W(distance(p.pos, np->pos));
    }
  }
}

void physics_update(World *w, double dt) {
  w->grid.build();

  compute_density(w);
}

void save_state(World *w, std::string filename) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Couldn't opern file to save state  file: " << filename << std::endl;
    exit(1);
  }

  uint32_t count = w->particles.size();
  file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

  for (auto& p: w->particles) {
    file.write(reinterpret_cast<char *>(&p.pos.x), sizeof(double));
    file.write(reinterpret_cast<char *>(&p.pos.y), sizeof(double));
    file.write(reinterpret_cast<char *>(&p.pressure), sizeof(double));
  }
  file.close();
}

World *initialize_world() {
  std::vector<Particle> *particles = new std::vector<Particle>();

  // Create particles at initial position
  for (double x = 2 * SPACING; x < 0.6; x += SPACING) {
    for (double y = 2 * SPACING; y < 2.0; y+= SPACING) {
      particles->push_back(make_particle(x / 1000, y));
    }
  }

  for (double x = 0; x < 3.1; x += SPACING) {
      particles->push_back(make_particle(x, 0));
      particles->push_back(make_particle(x, SPACING));
  }

  for (double y = 2 * SPACING; y < 2.0; y += SPACING) {
    particles->push_back(make_particle(0, y));
    particles->push_back(make_particle(SPACING, y));
    particles->push_back(make_particle(3, y));
    particles->push_back(make_particle(3 - SPACING, y));
  }

  World *world = new World(*particles);
  world->grid.build();

  // Compute initial mass
  for (auto& p: world->particles) {
    NeighbourIterator it = world->grid.get_neighbours(p);
    double sumW = 0.0;
    it.start();
    while (it.has_next()) {
      Particle *np = it.next();
      sumW += W(distance(p.pos, np->pos));
    }

    p.mass = 1000.0 / sumW;
  }

  return world;
}

int main(void) {
  World *world = initialize_world();

  physics_update(world, 0.01);

  save_state(world, "out/results.data");

  return 0;
}
