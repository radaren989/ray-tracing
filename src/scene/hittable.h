#ifndef HITTABLE_H
#define HITTABLE_H
//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related and neighboring rights to this software to the public domain
// worldwide. This software is distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public
// Domain Dedication along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

#include "../math/rtweekend.h"
#include "../math/aabb.h"

#include <array>

class material;

class hit_record {
public:
  point3 p;
  vec3 normal;
  shared_ptr<material> mat;
  double t;
  bool front_face;

  void set_face_normal(const ray &r, const vec3 &outward_normal);
};

using packet_hit_records = std::array<hit_record, ray_packet_width>;

class hittable {
public:
  virtual ~hittable() = default;

  virtual bool hit(const ray &r, interval ray_t, hit_record &rec) const = 0;
  virtual std::array<bool, ray_packet_width>
  hit_packet_records(const ray_packet &rays, interval ray_t,
                     packet_hit_records &records,
                     const std::array<bool, ray_packet_width> &active) const {
    std::array<bool, ray_packet_width> hits{};
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      if (active[lane]) {
        hits[lane] = hit(rays.ray_at(lane), ray_t, records[lane]);
      }
    }
    return hits;
  }

  virtual std::array<bool, ray_packet_width>
  hit_packet(const ray_packet &rays, interval ray_t) const {
    std::array<bool, ray_packet_width> active{};
    active.fill(true);
    return hit_packet_active(rays, ray_t, active);
  }
  virtual std::array<bool, ray_packet_width>
  hit_packet_active(
      const ray_packet &rays, interval ray_t,
      const std::array<bool, ray_packet_width> &active) const {
    std::array<bool, ray_packet_width> hits{};
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      if (active[lane]) {
        hit_record record;
        hits[lane] = hit(rays.ray_at(lane), ray_t, record);
      }
    }
    return hits;
  }
  virtual aabb bounding_box() const = 0;
};

#endif
