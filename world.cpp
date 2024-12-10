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

void World::print_logs() {
  for (auto log: logs) {
    printf("[%s %f] ", log.first.c_str(), log.second);
  }
  if (logs.size() > 0) printf("\n");
}

void World::print_timings() {
  for (auto log: timings) {
    Timing timing = log.second;
    if (timing.get_mean() >= 1000) {
      printf("[%s %4.1fms(± %.0f)] ", log.first.c_str(), (double)timing.get_current() / 1000, (double) timing.get_std() / 1000);
    } else {
      printf("[%s %4lluus (± %.0f)] ", log.first.c_str(), log.second.get_current(), log.second.get_std());
    }
  }
  if (timings.size() > 0) printf("\n");
}

void write_single(std::ofstream &file, float s) {
  file.write(reinterpret_cast<char *>(&s), sizeof(float));
}

void write_byte(std::ofstream &file, uint8_t byte) {
  file.write(reinterpret_cast<const char *>(&byte), sizeof(uint8_t));
}


void World::write_headers(std::ofstream &file, uint8_t flags) {
  // Write endianness
  uint8_t little_endian = (std::endian::native == std::endian::little) ? SIM_LITTLE_ENDIAN : 0;
  output_flags = (flags & 0b11111110) | little_endian;

  printf("Flags = %d\n", output_flags);
  write_byte(file, output_flags);

  // Count of particles
  uint32_t count = particles.size();
  printf("Count: %d\n", count);
  file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

  // Mass of particles
  if (output_flags & SIM_MASS) {
    for (Particle& p: particles) {
      write_single(file, p.mass);
    }
  }

  // Boundary or Not
  if (output_flags & SIM_BOUNDARY) {
    for (Particle& p: particles) {
      write_byte(file, p.boundary_particle);
    }
  }
}

void World::write_frame(std::ofstream &file) {
  uint8_t next_frame = 1;
  file.write(reinterpret_cast<char *>(&next_frame), sizeof(uint8_t));
  write_single(file, time);
  double *P = alg->get_pressure();
  for (auto &p : particles) {
    write_single(file, p.pos.x);
    write_single(file, p.pos.y);
    if (output_flags & SIM_PRESSURE) write_single(file, P[p.idx]);
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
