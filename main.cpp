#include <cassert>
#include <chrono>
#include <fstream>
#include <algorithm>
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

World *initialize_world(std::string filename, int parsing_scale) {
  std::vector<Particle> particles = parse_input_file(filename, parsing_scale);
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
  bool data_file_out;
  bool terminal_render;
  int parsing_scale;
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

bool find_arg(std::vector<std::string> args, std::string param) {
   auto loc = std::find(args.begin() + 1, args.end(), param);
   return loc < args.end();
}

void print_help() {
  using namespace std;

  cout << "SPH Fuild Simulator" << endl;
  cout << "simulator <input_filename> [optional parameters]" << endl << endl;
  cout << "--output    Path to simulation result ouptut file" << endl;
  cout << "--time      Target time for simulation" << endl;
  cout << "--iters     Target iterations of physics update (default 200)" << endl;
  cout << "             Not used if --time is provided" << endl;
  cout << "--no-render Disable rendering to terminal" << endl;
  cout << "--no-output Don't save results to file" << endl;
  cout << "--scale     Scale to use for Input file" << endl;
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

  std::string scale_str = get_arg(args, "--scale");
  if (scale_str == "") {
    params.parsing_scale = 1;
  } else {
    params.parsing_scale = std::max(1, std::stoi(scale_str));
  }

  if (find_arg(args, "--no-render")) {
    params.terminal_render = false;
  } else {
    params.terminal_render = true;
  }

  if (find_arg(args, "--no-output")) {
    params.data_file_out = false;
  } else {
    params.data_file_out = true;
  }

  return params;
}

int main(int argc, char** argv) {
  std::chrono::time_point start_point = std::chrono::high_resolution_clock::now();
  // Read args
  Params params = parse_args(argc, argv);
  // Initialize
  World *world = initialize_world(params.input_filename, params.parsing_scale);
  // Open output file
  std::ofstream file;
  if (params.data_file_out) {
    file.open(params.output_filename, std::ios::binary);
    if (!file) {
      std::cerr << "Couldn't opern file to save state  file: " << params.output_filename << std::endl;
      exit(1);
    }
    world->write_headers(file);
  }

  // Run simulation
  int iters = 0;
  while ((params.iters < 0 || (iters < params.iters)) &&
         (params.target_time < 0 || world->time < params.target_time)) {
    iters++;
    world->physics_update();

    if (params.terminal_render) render_to_terminal(world);

    std::chrono::duration duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_point);
    printf("[Iters: %d/%d] [Time: %.4fs/%.2f] [Wall Time: %.4fs]\n", iters, params.iters, world->time, params.target_time, (double) duration.count() / 1000);

    if (params.data_file_out) world->write_frame(file);
  }
  // Close output file
  if (params.data_file_out) world->write_footers(file);
  file.close();
  return 0;
}
