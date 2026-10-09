#include "../scene/bvh_node.h"
#include "../scene/hittable_list.h"
#include "../scene/material.h"
#include "../scene/sphere.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <array>

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
  const aabb packet_box(point3(-1, -1, -5), point3(1, 1, -2));
  std::array<ray, ray_packet_width> packet_rays = {
      ray(point3(0, 0, 0), vec3(0, 0, -1)),
      ray(point3(2, 0, 0), vec3(0, 0, -1)),
      ray(point3(0, 0, 0), vec3(0, 0, 1)),
      ray(point3(0, 0, 0), vec3(1, 0, -3)),
      ray(point3(0, 2, 0), vec3(0, -1, -3)),
      ray(point3(0, 0, -3), vec3(0, 0, 1)),
      ray(point3(-0.5, 0.5, 0), vec3(0, 0, -1)),
      ray(point3(0, 0, 0), vec3(0, 0, -0.0000000000001)),
  };
  const ray_packet packet(packet_rays);
  const auto packet_hits = packet_box.hit_packet(packet, interval(0.001, infinity));
  const auto avx2_hits =
      packet_box.hit_packet_avx2(packet, interval(0.001, infinity));
  for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
    if (packet_hits[lane] != packet_box.hit(packet_rays[lane],
                                             interval(0.001, infinity))) {
      std::cerr << "Packet AABB mismatch at lane " << lane << '\n';
      return EXIT_FAILURE;
    }
    if (avx2_hits[lane] != packet_hits[lane]) {
      std::cerr << "AVX2 packet AABB mismatch at lane " << lane << '\n';
      return EXIT_FAILURE;
    }
  }
  std::cout << "Packet AABB correctness passed for " << ray_packet_width
            << " lanes (scalar and AVX2)\n";

  auto packet_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
  const sphere packet_sphere(point3(0, 0, -3), 1, packet_material);
  const auto sphere_hits =
      packet_sphere.hit_packet(packet, interval(0.001, infinity));
  for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
    hit_record record;
    if (sphere_hits[lane] !=
        packet_sphere.hit(packet_rays[lane], interval(0.001, infinity),
                          record)) {
      std::cerr << "Packet sphere mismatch at lane " << lane << '\n';
      return EXIT_FAILURE;
    }
  }
  std::cout << "Packet sphere correctness passed for " << ray_packet_width
            << " lanes\n";

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
