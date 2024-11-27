#include "types.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <bit>
#include <iostream>
#include <utility>
#include <chrono>

Timing::Timing() {
  current = 0;
  sum_x = 0;
  sum_x2 = 0;
  count = 0;
  started = false;
}

void Timing::add(uint64_t value) {
  current = value;
  sum_x += value;
  sum_x2 += value * value;
  count ++;
}

void Timing::start() {
  start_point = std::chrono::high_resolution_clock::now();
  started = true;
}

void Timing::end() {
  if (!started) {
    std::cerr << "Timer not started! " << std::endl;
    exit(1);
  }
  auto end_point = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_point - start_point);
  add(duration.count());
  started = false;
}

uint64_t Timing::get_current() {
  return current;
}

double Timing::get_mean() {
  if (count == 0) return 0.0;
  return (double) sum_x / count;
}

double Timing::get_std() {
  if (count == 0) return 0.0;

  double mean = ((double)sum_x / count);
  return std::sqrt((double)sum_x2 / count - mean * mean);
}

World::World(std::vector<Particle> _particles, Algorithm *_alg) {
  particles = _particles;
  alg = _alg;
  grid = new Grid(&particles);
  logs = std::vector<std::pair<std::string, double>>();
}

void World::timer_start(std::string name) {
  if (timings.find(name) != timings.end()) {
    timings[name].start();
  } else {
    Timing t;
    timings[name] = t;
    timings[name].start();
  }
}

void World::timer_end(std::string name) {
  if (timings.find(name) != timings.end()) {
    timings[name].end();
  } else {
    std::cerr << "Timer hasn't been created for " << name << std::endl;
    exit(1);
  }
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

void World::physics_update() {
  logs.clear();
  timer_start("Physics");
  time += alg->physics_update();
  timer_end("Physics");
}

void World::log(std::string param, double value) {
  logs.push_back(std::pair(param, value));
}

uint8_t SIM_LITTLE_ENDIAN = 0b00001;
uint8_t SIM_MASS          = 0b00010;
uint8_t SIM_BOUNDARY      = 0b00100;
uint8_t SIM_PRESSURE      = 0b01000;
uint8_t SIM_VELOCITY      = 0b10000;

void write_single(std::ofstream &file, float s) {
  file.write(reinterpret_cast<char *>(&s), sizeof(float));
}

void write_byte(std::ofstream &file, uint8_t byte) {
  file.write(reinterpret_cast<const char *>(&byte), sizeof(uint8_t));
}


void World::write_headers(std::ofstream &file) {
  // Write endianness
  uint8_t little_endian = (std::endian::native == std::endian::little) ? SIM_LITTLE_ENDIAN : 0;
  uint8_t flags = little_endian | SIM_MASS | SIM_BOUNDARY;

  printf("Flags = %d\n", flags);
  write_byte(file, flags);

  // Count of particles
  uint32_t count = particles.size();
  printf("Count: %d\n", count);
  file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

  // Mass of particles
  for (Particle& p: particles) {
    write_single(file, p.mass);
  }

  // Boundary or Not
  for (Particle& p: particles) {
    write_byte(file, p.boundary_particle);
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
    assert(!std::isnan(p.pos.x));
    assert(!std::isnan(p.pos.y));
    assert(!std::isnan(p.vel.x));
    assert(!std::isnan(p.vel.y));
  }
}
