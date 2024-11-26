#include "types.h"
#include "kernel.h"
#include <cassert>
#include <cmath>
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

uint8_t SIM_LITTLE_ENDIAN = 0b00001;
uint8_t SIM_MASS          = 0b00010;
uint8_t SIM_PRESSURE      = 0b00100;
uint8_t SIM_VELOCITY      = 0b01000;

void write_single(std::ofstream &file, float s) {
  file.write(reinterpret_cast<char *>(&s), sizeof(float));
}

void World::write_headers(std::ofstream &file) {
  // Write endianness
  uint8_t little_endian = (std::endian::native == std::endian::little) ? SIM_LITTLE_ENDIAN : 0;
  uint8_t flags = little_endian | SIM_MASS;

  printf("Little Endian = %d\n", flags);
  file.write(reinterpret_cast<const char *>(&flags), sizeof(uint8_t));

  // Count of particles
  uint32_t count = particles.size();
  printf("Count: %d\n", count);
  file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

  // Mass of particles
  for (Particle& p: particles) {
    write_single(file, p.mass);
  }
}

void World::write_frame(std::ofstream &file) {
  uint8_t next_frame = 1;
  file.write(reinterpret_cast<char *>(&next_frame), sizeof(uint8_t));
  write_single(file, time);
  for (auto &p : particles) {
    write_single(file, p.pos.x);
    write_single(file, p.pos.y);
    //write_single(file, p.pressure);
  }
}

void World::write_footers(std::ofstream &file) {
  uint8_t next_frame = 0;
  file.write(reinterpret_cast<char *>(&next_frame), sizeof(uint8_t));
}

void World::sanity_checks() {
  // Check for NaNs
  for (auto p: particles) {
    assert(!isnan(p.pos.x));
    assert(!isnan(p.pos.y));
    assert(!isnan(p.vel.x));
    assert(!isnan(p.vel.y));
    assert(!isnan(p.vel_pred.x));
    assert(!isnan(p.vel_pred.y));
    assert(!isnan(p.alpha));
  }
}
