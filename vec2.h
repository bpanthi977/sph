#ifndef __vec2
#define __vec2

// C++ style
struct vec2 {
    double x;
    double y;
    vec2& operator+=(const vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
};
vec2 operator+(vec2 p1, vec2 p2);
vec2 operator-(vec2 p1, vec2 p2);
vec2 operator-(vec2 p1);
vec2 operator*(vec2 p1, double scalar);
vec2 operator*(double scalar, vec2 p1);
double dot(vec2 p1, vec2 p2);
double norm_square(vec2 p);
double norm(vec2 p);
double distance(vec2 p1, vec2 p2);

#endif
