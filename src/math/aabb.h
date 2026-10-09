#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "ray.h"

class aabb {
public:
  interval x, y, z;

  aabb();
  aabb(const interval &x, const interval &y, const interval &z);
  aabb(const point3 &minimum, const point3 &maximum);

  const interval &axis_interval(int axis) const;
  bool hit(const ray &r, interval ray_t) const;
};

aabb surrounding_box(const aabb &first, const aabb &second);

#endif
