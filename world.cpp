#include "types.h"
#include "kernel.h"
#include <cstdint>
#include <fstream>

World::World(std::vector<Particle> _particles, Algorithm _alg) {
  particles = _particles;
  alg = _alg;
  grid = new Grid(&particles);
}

vec2 World::viscous_acceleration(Particle &p) {
  vec2 acc = {0, 0};
  return acc;
}

vec2 World::external_acceleration(Particle &p) {
  vec2 acc = {0, 0};
  if (!p.boundary_particle) {
    acc.y = -9.81; // Gravity
  }
  return acc;
}

void World::write_headers(std::ofstream &file) {
  // Write endianness
  uint8_t flags = (std::endian::native == std::endian::little) ? 1 : 0;
  printf("Little Endian = %d\n", flags);
  file.write(reinterpret_cast<const char *>(&flags), sizeof(uint8_t));

  // Count of particles
  uint32_t count = particles.size();
  printf("Count: %d\n", count);
  file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));
}

void World::write_frame(std::ofstream &file) {
  uint8_t next_frame = 1;
  file.write(reinterpret_cast<char *>(&next_frame), sizeof(uint8_t));
  for (auto &p : particles) {
    file.write(reinterpret_cast<char *>(&p.pos.x), sizeof(double));
    file.write(reinterpret_cast<char *>(&p.pos.y), sizeof(double));
    file.write(reinterpret_cast<char *>(&p.pressure), sizeof(double));
  }
}

void World::write_footers(std::ofstream &file) {
  uint8_t next_frame = 0;
  file.write(reinterpret_cast<char *>(&next_frame), sizeof(uint8_t));
}
