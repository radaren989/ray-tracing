#include "../scene/bvh_node.h"
#include "../scene/hittable_list.h"
#include "../scene/material.h"
#include "../scene/sphere.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
bool same_hit(const hittable &linear, const hittable &bvh, const ray &r) {
  hit_record linear_record;
  hit_record bvh_record;
  const interval ray_t(0.001, infinity);

  const bool linear_hit = linear.hit(r, ray_t, linear_record);
  const bool bvh_hit = bvh.hit(r, ray_t, bvh_record);
  if (linear_hit != bvh_hit) {
    return false;
  }

  return !linear_hit ||
         std::abs(linear_record.t - bvh_record.t) < 1e-12;
}
} // namespace

int main() {
  hittable_list linear;
  auto material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
  const std::vector<std::pair<point3, double>> spheres = {
      {point3(0, 0, -1), 0.5},
      {point3(-1.5, 0.2, -3), 0.7},
      {point3(1.4, -0.2, -2.5), 0.4},
      {point3(0, -100.5, -1), 100},
      {point3(0, 1.5, -4), 0.8},
  };

  for (const auto &[center, radius] : spheres) {
    linear.add(make_shared<sphere>(center, radius, material));
  }
  bvh_node bvh(linear.objects);

  int tested_rays = 0;
  for (int y = -6; y <= 6; ++y) {
    for (int x = -8; x <= 8; ++x) {
      const ray r(point3(0, 0, 0),
                  unit_vector(vec3(x * 0.2, y * 0.2, -1)));
      if (!same_hit(linear, bvh, r)) {
        std::cerr << "BVH mismatch for ray (" << x << ", " << y << ")\n";
        return EXIT_FAILURE;
      }
      ++tested_rays;
    }
  }

  std::cout << "BVH correctness passed for " << tested_rays << " rays\n";
}
