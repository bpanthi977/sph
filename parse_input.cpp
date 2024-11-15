#include "types.h"
#include <iostream>
#include <fstream>
#include <vector>

using std::istream;

std::vector<Particle> parse_input_file(std::string filename) {
  std::ifstream file(filename);
  char ch;
  std::vector<Particle> particles;
  double x = 0, y = 0;

  if (!file) {
    std::cerr << "Couldn't open file" << std::endl;
    exit(1);
  }

  while (!file.eof()) {
    file.read(&ch, 1);
    printf("%c", ch);
    if (ch == '#') {
      Particle p = {0};
      p.pos.x = x;
      p.pos.y = y;
      p.boundary_particle = true;
      particles.push_back(p);
    } else if (ch == '.') {
      Particle p;
      p.pos.x = x;
      p.pos.y = y;
      p.boundary_particle = false;
      particles.push_back(p);
    }

    if (ch == '\n') {
      y = y - SPACING;
      x = 0;
    } else {
      x = x + SPACING;
    }
  }

  file.close();
  return particles;
}
