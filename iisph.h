#ifndef __IISPH
#define __IISPH

#include "types.h"

class IISPH: public Algorithm {
private:
  double *pressure;
  World *w;

public:
  virtual double *get_pressure();
  virtual void initialize(World *w);
  virtual double physics_update();
};
#endif
