#include "vec2.h"
#include "types.h"

// Kernel Function
double W(double r) {
  const double normalization = 15.0 / (14 * PI * std::pow((SUPPORT_RADIUS / 2), 2));
  float q = 2 * r / SUPPORT_RADIUS;

  if (0 <= q && q <= 1)
    return normalization * (1.0 / 6 * pow(2 - q, 3) - 4.0 / 6 * pow(1 - q, 3));
  else if (1 <= q && q <= 2)
    return normalization * (1.0/6 * pow(2-q, 3));

  return 0.0;
}

// First derivative of Kernel function W(r)
double dW_dr(double r) {
  const double normalization =  15.0 / (14 * PI * std::pow((SUPPORT_RADIUS / 2), 2));
  float q = 2 * r / SUPPORT_RADIUS;

  if (0 <= q && q <= 1)
    return normalization * 0.5 * (4 * std::pow(1 - q, 2) - std::pow(2 - 1, 2));
  else if (1 <= q && q <= 2)
    return normalization * -0.5 * std::pow(2 - q, 2);

  return 0.0;
}

vec2 gradW(vec2 p1, vec2 p2) {
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  double r = std::sqrt(dx * dx + dy * dy);
  double dW = dW_dr(r);
  vec2 result = {
    .x = dW / r * dx,
    .y = dW / r * dy
  };
  return result;
}
