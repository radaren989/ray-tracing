#ifndef AABB_H
#define AABB_H

#include "interval.h"
#include "ray.h"
#include "ray_packet.h"

#include <array>

class aabb {
public:
  interval x, y, z;

  aabb();
  aabb(const interval &x, const interval &y, const interval &z);
  aabb(const point3 &minimum, const point3 &maximum);

  const interval &axis_interval(int axis) const {
    switch (axis) {
    case 0:
      return x;
    case 1:
      return y;
    default:
      return z;
    }
  }
  bool hit(const ray &r, interval ray_t) const;
  std::array<bool, ray_packet_width>
  hit_packet(const ray_packet &rays, interval ray_t) const;
  std::array<bool, ray_packet_width>
  hit_packet_avx2(const ray_packet &rays, interval ray_t) const;
};

aabb surrounding_box(const aabb &first, const aabb &second);

#endif
