#ifndef __SPH_TYPES
#define __SPH_TYPES

#include "vec2.h"
#include <map>
#include <unordered_map>
#include <vector>

typedef struct {
  vec2 pos;
  vec2 vel;
  double mass;

  double rho;
  double pressure;
  double aii;
  double si;
  double ai[2];
  bool boundary_particle;
} Particle;

const double SUPPORT_RADIUS = 1.0 / 24;
const double SPACING = SUPPORT_RADIUS / 1.2;
const double PI = 3.1415926539;

typedef struct {
  int x;
  int y;
} GridId;

typedef struct {
  GridId grid_id;
  std::vector<Particle*> particles;
} GridBox;

class NeighbourIterator;

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
  Grid(std::vector<Particle> &particles);
  void build();
  NeighbourIterator get_neighbours(Particle& p);
};

class NeighbourIterator {
  Grid *grid;
  Particle *particle;
  GridId particle_grid_id;
  int grid_iter_i; // 0 to 8 representing neighbouring cells
  std::vector<Particle*>::const_iterator particle_iter;
  std::vector<Particle*>::const_iterator particle_iter_end;

  bool find_next_grid();
 public:
  NeighbourIterator(Grid& g, Particle &p);
  bool has_next();
  Particle *next();
  void start();
};

class World {
  public:
  std::vector<Particle> particles;
  Grid grid;

  World(std::vector<Particle> particles);
  void write_headers(std::ofstream &file);
  void write_frame(std::ofstream &file);
  void write_footers(std::ofstream &file);
};

std::vector<Particle> parse_input_file(std::string filename);

#endif
