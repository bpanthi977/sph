#include <_types/_uint32_t.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <arpa/inet.h>
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
    for (Particle *np: w->grid.get_neighbours(&p)) {
      p.rho += np->mass * W(distance(p.pos, np->pos));
    }
  }
}

vec2 viscous_acceleration(World *w, Particle &p) {
  vec2 acc = {0, 0};
  return acc;
}

vec2 external_acceleration(World *w, Particle &p) {
  vec2 acc = {0, 0};
  if (!p.boundary_particle) {
    acc.y = -9.81; // Gravity
  }
  return acc;
}

void physics_update(World *w, double dt) {
  w->grid.build();
  compute_density(w);

  // Compute forces
  //  due to external forces and viscous forces
  for (Particle &p: w->particles) {
    p.vel += (viscous_acceleration(w, p) + external_acceleration(w, p)) * dt;
  }

  // Compute pressure force & add to velociy


  // Apply change in position
  for (Particle& p: w->particles) {
    if (!p.boundary_particle) {
      p.pos += p.vel * dt;
    }
  }
}

void setup_initial_mass(World *world) {
  // Compute initial mass
  for (auto& p: world->particles) {
    double sumW = 0.0;
    for (Particle *np: world->grid.get_neighbours(&p)) {
      sumW += W(distance(p.pos, np->pos));
    }

    p.mass = 1000.0 / sumW;
  }
}

World *initialize_world(std::string filename) {
  std::vector<Particle> particles = parse_input_file(filename);
  World *world = new World(particles);
  world->grid.build();
  setup_initial_mass(world);
  return world;
}

int main(int argc, char** argv) {
  /// Initialization
  // initialize world
  std::string input_filename = "input2.txt";
  if (argc >= 2) {
    input_filename = argv[1];
  }
  World *world = initialize_world(input_filename);
  // open output file
  std::string filename = "out/results.data";
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Couldn't opern file to save state  file: " << filename << std::endl;
    exit(1);
  }
  world->write_headers(file);

  for (int i=0; i<2; i++) {
    // Run simulation
    physics_update(world, 0.01);
    world->write_frame(file);
  }

  /// End
  world->write_footers(file);
  file.close();

  return 0;
}
