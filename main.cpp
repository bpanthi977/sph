#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include "vec2.h"
#include "types.h"
#include "kernel.h"
#include "iisph.h"

void setup_initial_mass(World *world) {
  // Compute initial mass
  for (Particle &p : world->particles) {
    double sumW = W(0);
    for (Particle *np: world->grid->get_neighbours(&p)) {
      sumW += W(distance(p.pos, np->pos));
    }
    p.mass = world->rho_0 / sumW;
    assert(p.mass >= 0);
  }
}

World *initialize_world(std::string filename) {
  std::vector<Particle> particles = parse_input_file(filename);
  World *w = new World(particles, IISPH());
  printf("World loaded [%zu particles]\n", w->particles.size());
  w->grid->build();
  setup_initial_mass(w);
  w->alg.initialize(w);
  return w;
}

int main(int argc, char** argv) {
  // initialize world
  std::string input_filename = "input3.txt";
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

  for (int i=0; i<2000; i++) {
    // Run simulation
    double dt = world->alg.physics_update(world);
    world->time += dt;

    render_to_terminal(world);
    world->write_frame(file);
  }

  /// End
  world->write_footers(file);
  file.close();
  return 0;
}
