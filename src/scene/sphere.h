#ifndef SPHERE_H
#define SPHERE_H
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

#include "../scene/hittable.h"

class sphere : public hittable {
public:
  sphere(const point3 &center, double radius, shared_ptr<material> mat);
  bool hit(const ray &r, interval ray_t, hit_record &rec) const override;
  std::array<bool, ray_packet_width>
  hit_packet_active(
      const ray_packet &rays, interval ray_t,
      const std::array<bool, ray_packet_width> &active) const override;
  aabb bounding_box() const override;

private:
  point3 center;
  double radius;
  shared_ptr<material> mat;
};

#endif
