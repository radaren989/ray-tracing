#include "aabb.h"

#include <algorithm>
#include <cmath>

aabb::aabb() = default;

aabb::aabb(const interval &x, const interval &y, const interval &z)
    : x(x), y(y), z(z) {}

aabb::aabb(const point3 &minimum, const point3 &maximum)
    : x(minimum.x(), maximum.x()), y(minimum.y(), maximum.y()),
      z(minimum.z(), maximum.z()) {}

bool aabb::hit(const ray &r, interval ray_t) const {
  const auto hit_axis = [&ray_t](const interval &axis_range, double origin,
                                 double direction) {
    if (std::fabs(direction) < 1e-12) {
      return axis_range.contains(origin);
    }

    auto t0 = (axis_range.min - origin) / direction;
    auto t1 = (axis_range.max - origin) / direction;
    if (t0 > t1) {
      std::swap(t0, t1);
    }

    ray_t.min = std::max(ray_t.min, t0);
    ray_t.max = std::min(ray_t.max, t1);
    return ray_t.max > ray_t.min;
  };

  return hit_axis(x, r.origin().x(), r.direction().x()) &&
         hit_axis(y, r.origin().y(), r.direction().y()) &&
         hit_axis(z, r.origin().z(), r.direction().z());
}

aabb surrounding_box(const aabb &first, const aabb &second) {
  const point3 minimum(
      std::min(first.x.min, second.x.min),
      std::min(first.y.min, second.y.min),
      std::min(first.z.min, second.z.min));
  const point3 maximum(
      std::max(first.x.max, second.x.max),
      std::max(first.y.max, second.y.max),
      std::max(first.z.max, second.z.max));
  return aabb(minimum, maximum);
}
