#ifndef RAY_PACKET_H
#define RAY_PACKET_H

#include "ray.h"

#include <array>
#include <cstddef>

constexpr std::size_t ray_packet_width = 8;

struct ray_packet {
  std::array<double, ray_packet_width> origin_x;
  std::array<double, ray_packet_width> origin_y;
  std::array<double, ray_packet_width> origin_z;
  std::array<double, ray_packet_width> direction_x;
  std::array<double, ray_packet_width> direction_y;
  std::array<double, ray_packet_width> direction_z;

  ray_packet() = default;

  ray ray_at(std::size_t lane) const {
    return ray(point3(origin_x[lane], origin_y[lane], origin_z[lane]),
               vec3(direction_x[lane], direction_y[lane], direction_z[lane]));
  }

  explicit ray_packet(const std::array<ray, ray_packet_width> &rays) {
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      origin_x[lane] = rays[lane].origin().x();
      origin_y[lane] = rays[lane].origin().y();
      origin_z[lane] = rays[lane].origin().z();
      direction_x[lane] = rays[lane].direction().x();
      direction_y[lane] = rays[lane].direction().y();
      direction_z[lane] = rays[lane].direction().z();
    }
  }
};

#endif
