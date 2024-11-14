#ifndef __vec2
#define __vec2

typedef struct {
  double x;
  double y;
} vec2;

vec2 operator+(vec2 p1, vec2 p2);
vec2 operator-(vec2 p1, vec2 p2);
vec2 operator*(vec2 p1, vec2 p2);
double dot(vec2 p1, vec2 p2);
double norm(vec2 p);
double distance(vec2 p1, vec2 p2);

#endif
