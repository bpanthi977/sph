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


typedef struct {
  std::string input_filename;
  std::string output_filename;
  int iters;
  double target_time;
} Params;

std::string get_arg(std::vector<std::string> args, std::string param) {
  auto loc = std::find(args.begin() + 1, args.end(), param);
  loc++;
  if (loc >= args.end()) {
    return "";
  } else {
    return *(loc);
  }
}

void print_help() {
  using namespace std;

  cout << "SPH Fuild Simulator" << endl;
  cout << "simulator <input_filename> [optional parameters]" << endl << endl;
  cout << "--output Path to simulation result ouptut file" << endl;
  cout << "--time   Target time for simulation" << endl;
  cout << "--iters  Target iterations of physics update (default 200)" << endl;
  cout << "           Not used if --time is provided" << endl;
  cout << "--help   Prints this help message." << endl;
}

Params parse_args(int argc, char** argv) {
  if (argc < 2) {
    print_help();
    exit(1);
  }
  std::vector<std::string> args;
  Params params;
  for (int i=0; i<argc; i++) {
    args.push_back(argv[i]);
  }

  params.input_filename = argv[1];

  if (params.input_filename == "--help" || get_arg(args, "--help") != "") {
    print_help();
    exit(0);
  }

  params.output_filename = get_arg(args, "--output");
  if (params.output_filename == "") {
    params.output_filename = params.input_filename + ".data";
  }

  std::string iters_str = get_arg(args, "--iters");
  if (iters_str == "") {
    params.iters = -1;
  } else {
    params.iters = std::stoi(iters_str);
  }

  std::string time_str = get_arg(args, "--time");
  if (time_str == "") {
    params.target_time = -1;
  } else {
    params.target_time = std::stod(time_str);
  }

  if (iters_str == "" && time_str == "") {
    params.iters = 200;
  }

  return params;
}

int main(int argc, char** argv) {
  // Read args
  Params params = parse_args(argc, argv);
  // Initialize
  World *world = initialize_world(params.input_filename);

  // Open output file
  std::ofstream file(params.output_filename, std::ios::binary);
  if (!file) {
    std::cerr << "Couldn't opern file to save state  file: " << params.output_filename << std::endl;
    exit(1);
  }
  world->write_headers(file);

  // Run simulation
  int iters = 0;
  while ((params.iters < 0 || (iters < params.iters)) &&
         (params.target_time < 0 || world->time < params.target_time)) {
    iters++;
    world->physics_update();

    render_to_terminal(world);
    printf("[Iters: %d/%d] [Time: %.4fs/%.2f]\n", iters, params.iters, world->time, params.target_time);

    world->write_frame(file);
  }
  // Close output file
  world->write_footers(file);
  file.close();
  return 0;
}
