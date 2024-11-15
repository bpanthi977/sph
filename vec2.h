#ifndef __vec2
#define __vec2

typedef struct {
  double x;
  double y;
} vec2;

void operator+=(vec2& self, const vec2& other);
vec2 operator+(vec2 p1, vec2 p2);
vec2 operator-(vec2 p1, vec2 p2);
vec2 operator*(vec2 p1, vec2 p2);
vec2 operator*(vec2 p1, double scalar);
double dot(vec2 p1, vec2 p2);
double norm(vec2 p);
double distance(vec2 p1, vec2 p2);

#endif
