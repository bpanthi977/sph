#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
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
  // Read args
  std::string input_filename;
  std::string output_filename;
  if (argc >= 2) {
    input_filename = argv[1];
  } else {
    std::cerr << "Please specify input file name" << std::endl;
    exit(1);
  }
  if (argc >= 3) {
    output_filename = argv[2];
  } else {
    output_filename = input_filename + ".data";
  }

  // Initialize
  World *world = initialize_world(input_filename);

  // Open output file
  std::ofstream file(output_filename, std::ios::binary);
  if (!file) {
    std::cerr << "Couldn't opern file to save state  file: " << output_filename << std::endl;
    exit(1);
  }
  world->write_headers(file);

  // Run simulation
  for (int i=0; i<2000; i++) {
    world->physics_update();

    render_to_terminal(world);
    world->write_frame(file);
  }

  // Close output file
  world->write_footers(file);
  file.close();
  return 0;
}
