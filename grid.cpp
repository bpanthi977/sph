#include "vec2.h"
#include "types.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

GridId grid_id(vec2 pos) {
  GridId result;
  result.x = floor(pos.x / (2 * SUPPORT_RADIUS));
  result.y = floor(pos.y / (2 * SUPPORT_RADIUS));
  return result;
}

bool grid_id_equal(GridId &g1, GridId &g2) {
  return g1.x == g2.x && g1.y == g2.y;
}

int grid_id_hash(GridId &id, uint64_t size) {
  uint64_t hash = ((uint64_t) abs(id.x) * 73856093 + (uint64_t) abs(id.y) * 19349663) %  size;
  assert(hash >= 0);
  return hash;
}

GridHashMap::GridHashMap(int size): size(size) {
  grid_hash_map = new GridBox[size];
}

void GridHashMap::clear() {
  for (int idx = 0; idx < size; idx++) {
    grid_hash_map[idx].used = false;
    grid_hash_map[idx].particles.clear();
    grid_hash_map[idx].grid_id.x = 0;
    grid_hash_map[idx].grid_id.y = 0;
  }
}

void GridHashMap::insert(Particle *p) {
  GridId id = grid_id(p->pos);
  GridBox *grid = find_grid(id);
  grid->particles.push_back(p);
}

GridBox *GridHashMap::find_grid(GridId grid_id) {
  int idx = grid_id_hash(grid_id, size);
  int start_idx = idx;
  GridBox *grid = grid_hash_map + idx;
  do {
    if (grid->used == false) {
      grid->grid_id = grid_id;
      grid->used = true;
      return grid;
    }

    if (grid_id_equal(grid->grid_id, grid_id)) {
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

Grid::Grid(std::vector<Particle> *ps): grid_hash_map(10 * ps->size()) {
  printf("Particles size %zu\n", ps->size());
  particles = ps;
}

void Grid::build() {
  grid_hash_map.clear();
  for (std::vector<Particle>::iterator it = particles->begin(); it != particles->end(); ++it) {
    Particle *p = &(*it);
    grid_hash_map.insert(p);
  }
}

Neighbours Grid::get_neighbours(Particle *p) {
  return Neighbours(this, p);
}

Neighbours::Neighbours(Grid *g, Particle *p) {
  grid = g;
  particle = p;
}

NeighbourIterator Neighbours::begin() {
  return NeighbourIterator(grid, particle, false);
};

NeighbourIterator Neighbours::end() {
  return NeighbourIterator(grid, particle, true);
}

NeighbourIterator::NeighbourIterator(Grid* g, Particle* p, bool _is_end) {
  grid = g;
  particle = p;
  is_end = _is_end;

  particle_grid_id = grid_id(p->pos);
  grid_iter_i = -1;
  find_next_grid();
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

    if (grid_box->particles.size() != 0) {
      particle_iter = grid_box->particles.begin();
      particle_iter_end = grid_box->particles.end();
      if (*particle_iter == particle) {
        particle_iter++;
      }
      if (particle_iter != particle_iter_end) {
        break;
      }
    }
  } while (true);

  return true;
}

NeighbourIterator& NeighbourIterator::operator++() {
  if (grid_iter_i == 9) {
     // No next particle
  } else {
    ++particle_iter;
    if (particle_iter != particle_iter_end && *particle_iter == particle) ++particle_iter;

    if (particle_iter == particle_iter_end)
      find_next_grid();
  }
  return *this;
}

Particle* NeighbourIterator::operator*() {
  return *particle_iter;
}


bool NeighbourIterator::operator==(const NeighbourIterator& other) const {
  return other.is_end && grid_iter_i == 9;
}

bool NeighbourIterator::operator!=(const NeighbourIterator& other) const {
  return !(*this == other);
}
