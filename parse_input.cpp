#include "types.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

using std::istream;

std::vector<Particle> parse_input_file(std::string filename, int scale) {
  std::ifstream file(filename);
  char ch;
  std::vector<Particle> particles;
  std::vector<char> boundary_chars;
  bool firstline = true;
  double x = 0, y = 0;

  if (!file) {
    std::cerr << "Couldn't open file" << std::endl;
    exit(1);
  }

  #define IS_BOUNDARY_CHAR std::find(boundary_chars.begin(), boundary_chars.end(), ch) != boundary_chars.end()

  while (!file.eof()) {
    file.read(&ch, 1);
    printf("%c", ch);
    if (ch == ' ' || ch == '\n') {
      // Ignore spaces
    } else if (firstline || IS_BOUNDARY_CHAR) {
      for (int ix=0; ix<scale; ix++) {
        for (int iy=0; iy<scale; iy++) {
          Particle p = {0};
          p.symbol = ch;
          p.idx = particles.size();
          p.pos = {x + SPACING * ix, y + SPACING * iy};
          p.vel = {0, 0};
          p.boundary_particle = true;
          particles.push_back(p);
          if (firstline && !(IS_BOUNDARY_CHAR)) {
            boundary_chars.push_back(ch);
          }
        }
      }
    } else {
      for (int ix=0; ix < scale; ix++) {
        for (int iy=0; iy < scale; iy++) {
          Particle p = {0};
          p.symbol = ch;
          p.idx = particles.size();
          p.pos = {x + SPACING * ix, y + SPACING * iy};
          p.vel = {0, 0};
          p.boundary_particle = false;
          particles.push_back(p);
        }
      }
    }

    if (ch == '\n') {
      firstline = false;
      y = y - SPACING * scale;
      x = 0;
    } else {
      x = x + SPACING * scale;
    }
  }

  file.close();
  return particles;
}

void render_to_terminal(World *w) {
  vec2 bounds_max = w->particles[0].pos;
  vec2 bounds_min = bounds_max;
  for (Particle& p: w->particles) {
    bounds_max.x = std::max(bounds_max.x, p.pos.x);
    bounds_max.y = std::max(bounds_max.y, p.pos.y);
    bounds_min.x = std::min(bounds_min.x, p.pos.x);
    bounds_min.y = std::min(bounds_min.y, p.pos.y);
  }

  // Initialize render buffer
  int x_size = std::ceil((bounds_max.x - bounds_min.x) / SPACING) + 1;
  int y_size = std::ceil((bounds_max.y - bounds_min.y) / SPACING) + 1;
  int total_size = x_size * y_size;
  std::unique_ptr<char[]> render_buffer(new char[total_size]);

  for (int i=0; i < total_size; i++) {
    render_buffer[i] = ' ';
  }

  for (Particle& p: w->particles) {
    int x = std::ceil((p.pos.x - bounds_min.x) / SPACING);
    int y = std::ceil((bounds_max.y - p.pos.y) / SPACING);
    render_buffer[y * x_size + x] = p.symbol;
  }

  printf("\033[2J"); // Clear the screen
  printf("\033[H");  // Move the cursor to the top-left corner
  for (int y = 0; y < y_size; y++) {
    for (int x = 0; x < x_size; x++) {
      printf("%c", render_buffer[y * x_size + x]);
    }
    printf("\n");
  }

  printf("\n");
}
