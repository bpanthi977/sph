#include "vec2.h"
#include "types.h"

// Kernel Function
double cubic_spline_2d(double r) {
  const double normalization = 40.0 / (7.0 * PI * std::pow(SUPPORT_RADIUS, 2));
  float q = r / SUPPORT_RADIUS;

  if (0 <= q && q <= 1)
    return normalization * (1 - 6 * q * q * (1 - q));
  else if (1 <= q && q <= 2)
    return normalization * 2 * std::pow(1-q, 3);

  return 0.0;
}

double W(double r) {
  return cubic_spline_2d(r);
}

// First derivative of Kernel function W(r)
double dW_dr(double r) {
  const double normalization = 30.0 / (7 * PI * std::pow(SUPPORT_RADIUS, 2));
  float q = 2 * r / SUPPORT_RADIUS;

  if (0 <= q && q <= 1)
    return normalization * 0.5 * (4 * std::pow(1 - q, 2) - std::pow(2 - q, 2));
  else if (1 <= q && q <= 2)
    return normalization * -0.5 * std::pow(2 - q, 2);

  return 0.0;
}

vec2 gradW(vec2 p1, vec2 p2) {
  vec2 r = p1 - p2;
  double rnorm = norm(r);

  if (rnorm < 1e-8) {
    vec2 result = {0, 0};
    return result;
  }
  return r * (dW_dr(rnorm) / rnorm);
}

double gradW_norm(vec2 p1, vec2 p2) {
   return dW_dr(norm(p1 - p2));
}
