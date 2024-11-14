#include <cmath>
#include "vec2.h"

vec2 operator+(vec2 p1, vec2 p2) {
  vec2 result = {.x = p1.x + p2.x , .y = p1.y + p2.y};
  return result;
}

vec2 operator-(vec2 p1, vec2 p2) {
  vec2 result = {.x = p1.x - p2.x , .y = p1.y - p2.y};
  return result;
}

vec2 operator*(vec2 p1, vec2 p2) {
  vec2 result = {.x = p1.x * p2.x , .y = p1.y * p2.y};
  return result;
}

double dot(vec2 p1, vec2 p2) {
  return p1.x * p2.x + p1.y * p2.y;
}

double norm(vec2 p) {
  return std::sqrt(p.x * p.x + p.y * p.y);
}

double distance(vec2 p1, vec2 p2) {
  return norm(p1 - p2);
}
