#include "vec2.h"
#include "types.h"
#include <iostream>

GridId grid_id(vec2 pos) {
  GridId result;
  result.x = fmod(pos.x, 2 * SUPPORT_RADIUS);
  result.y = fmod(pos.y, 2 * SUPPORT_RADIUS);
  return result;
}

bool grid_id_equal(GridId &g1, GridId &g2) {
  return g1.x == g2.x && g1.y == g2.y;
}

bool grid_id_zero(GridId &g) {
  return g.x == 0 && g.y == 0;
}

int grid_id_hash(GridId &id, int size) {
  int hash = (abs(id.x) * 73856093 + abs(id.y) * 19349663) % size;
  return hash;
}

GridHashMap::GridHashMap(int size): size(size) {
  grid_hash_map = new GridBox[size];
}

void GridHashMap::clear() {
  for (int idx = 0; idx < size; idx++) {
    grid_hash_map[idx].particles.clear();
    grid_hash_map[idx].grid_id.x = 0;
    grid_hash_map[idx].grid_id.y = 0;
  }
}

void GridHashMap::insert(Particle *p) {
  GridId id = grid_id(p->pos);
  GridBox *grid = find_grid(id);
  if (grid->grid_id.x == 0 && grid->grid_id.y == 0) { // Unintialized
    grid->grid_id = id;
  }

  grid->particles.push_back(p);
}

GridBox *GridHashMap::find_grid(GridId grid_id) {
  int idx = grid_id_hash(grid_id, size);
  int start_idx = idx;
  GridBox *grid = grid_hash_map + idx;
  do {
    if (grid_id_equal(grid->grid_id, grid_id)) {
      return grid;
    }
    if (grid_id_zero(grid->grid_id)) {
      return grid;
    }
    idx = (idx + 1) % size;
    grid = grid_hash_map + idx;
    if (idx == start_idx) {
      std::cerr << "Hash map full. Panic!" << std::endl;
      exit(1);
    }
  } while (true);
}

Grid::Grid(std::vector<Particle> &ps): grid_hash_map(100) {
  particles = &ps;
}

void Grid::build() {
  grid_hash_map.clear();
  for (std::vector<Particle>::iterator it = particles->begin(); it != particles->end(); ++it) {
    Particle *p = &(*it);
    grid_hash_map.insert(p);
  }
}

NeighbourIterator Grid::get_neighbours(Particle& p) {
  return NeighbourIterator(*this, p);
}

NeighbourIterator::NeighbourIterator(Grid& g, Particle& p) {
  grid = &g;
  particle = &p;
  particle_grid_id = grid_id(p.pos);
}

const GridId grid_neigbour_idx_offsets[9] = {{-1, -1}, {0, -1}, {1, -1},
                                             {-1, 0},  {0, 0},  {1, 0},
                                             {-1, 1},  {0, 1},  {1, 1}};

bool NeighbourIterator::find_next_grid() {
  if (grid_iter_i == 9) {
    return false;
  }

  const GridBox *grid_box;
  do {
    grid_iter_i++;
    if (grid_iter_i == 9) {
      return false;
    }

    GridId g = particle_grid_id;
    g.x += grid_neigbour_idx_offsets[grid_iter_i].x;
    g.y += grid_neigbour_idx_offsets[grid_iter_i].y;

    grid_box = grid->grid_hash_map.find_grid(g);
  } while (grid_box->particles.size() == 0);

  particle_iter = grid_box->particles.begin();
  particle_iter_end = grid_box->particles.end();
  return true;
}

void NeighbourIterator::start() {
  grid_iter_i = -1;
  find_next_grid();
}

bool NeighbourIterator::has_next() {
  if (grid_iter_i == 9) return false;
  if (particle_iter == particle_iter_end) {
    return find_next_grid();
  }

  return true;
}

Particle* NeighbourIterator::next() {
  Particle* particle = *particle_iter;
  ++particle_iter;
  return particle;
}
