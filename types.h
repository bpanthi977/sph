#ifndef __SPH_TYPES
#define __SPH_TYPES

#include "vec2.h"
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

typedef struct Particle {
  int idx;
  char symbol;
  vec2 pos;
  vec2 vel;
  double mass;

  double rho;
  bool boundary_particle;
} Particle;

const double SUPPORT_RADIUS = 1.0 / 24;
const double SPACING = SUPPORT_RADIUS / 1.2;
const double PI = 3.1415926539;

typedef struct {
  int x;
  int y;
} GridId;

typedef struct GridBox {
  GridId grid_id;
  std::vector<Particle*> particles;
  bool used;

  GridBox(): grid_id({0}), particles(std::vector<Particle*>()), used(false) {}
} GridBox;

class Neighbours;

class GridHashMap {
  GridBox *grid_hash_map;
  int size;
  public:
  GridHashMap(int size);
  void clear();
  void insert(Particle *p);
  GridBox* find_grid(GridId grid_id);
};

class Grid {
  std::vector<Particle> *particles;
  GridHashMap grid_hash_map;
  friend class NeighbourIterator;

 public:
  Grid(std::vector<Particle> *particles);
  void build();
  Neighbours get_neighbours(Particle *p);
};

class NeighbourIterator{
  Grid *grid;
  Particle *particle;

  GridId particle_grid_id;
  int grid_iter_i; // 0 to 8 representing neighbouring cells
  // iterators inside the grid
  std::vector<Particle*>::const_iterator particle_iter;
  std::vector<Particle*>::const_iterator particle_iter_end;

  bool is_end;
  bool find_next_grid();
public:
  NeighbourIterator(Grid *g, Particle *p, bool is_end);
  // Equality comparison (with end)
  bool operator==(const NeighbourIterator& other) const;
  // Inequality comparision (with end)
  bool operator!=(const NeighbourIterator& other) const;
  // Pre-increment operator
  NeighbourIterator& operator++();
  // Dereference
  Particle* operator*();
};

class Neighbours {
  Grid *grid;
  Particle *particle;
public:

  Neighbours(Grid *g, Particle *p);
  NeighbourIterator begin();
  NeighbourIterator end();
};

class World;

struct Algorithm {
  void (*initialize)(World* w);
  double (*physics_update)(World *w);
};

class Timing {
private:
  int current;
  uint64_t sum_x;
  uint64_t sum_x2;
  int count;
  std::chrono::high_resolution_clock::time_point start_point;
  bool started;
  void add(uint64_t value);
public:
  Timing();
  uint64_t get_current();
  double get_std();
  double get_mean();
  void start();
  void end();
};

class World {

public:
  double rho_0 = 1000.0;
  double time = 0.0;
  std::vector<Particle> particles;
  Grid *grid;
  Algorithm alg;
  std::vector<std::pair<std::string, double>> logs;
  std::unordered_map<std::string, Timing> timings;

  World(std::vector<Particle> particles, Algorithm alg);

  vec2 viscous_acceleration(Particle &p);
  vec2 external_acceleration(Particle &p);
  void physics_update();

  // Logging
  void log(std::string param, double value);
  void timer_start(std::string name);
  void timer_end(std::string name);

  // Save to file
  void write_headers(std::ofstream &file);
  void write_frame(std::ofstream &file);
  void write_footers(std::ofstream &file);

  // Debugging
  void sanity_checks();
};


std::vector<Particle> parse_input_file(std::string filename);
void render_to_terminal(World *w);


#endif
