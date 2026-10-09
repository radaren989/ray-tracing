#include "aabb.h"

#include <algorithm>
#include <array>
#include <cmath>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

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

std::array<bool, ray_packet_width>
aabb::hit_packet(const ray_packet &rays, interval ray_t) const {
  std::array<bool, ray_packet_width> hits{};
  for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
    interval lane_ray_t = ray_t;
    auto hit_axis = [&lane_ray_t](const interval &axis_range, double origin,
                                  double direction) {
      if (std::fabs(direction) < 1e-12) {
        return axis_range.contains(origin);
      }

      auto t0 = (axis_range.min - origin) / direction;
      auto t1 = (axis_range.max - origin) / direction;
      if (t0 > t1) {
        std::swap(t0, t1);
      }

      lane_ray_t.min = std::max(lane_ray_t.min, t0);
      lane_ray_t.max = std::min(lane_ray_t.max, t1);
      return lane_ray_t.max > lane_ray_t.min;
    };

    hits[lane] =
        hit_axis(x, rays.origin_x[lane], rays.direction_x[lane]) &&
        hit_axis(y, rays.origin_y[lane], rays.direction_y[lane]) &&
        hit_axis(z, rays.origin_z[lane], rays.direction_z[lane]);
  }
  return hits;
}

std::array<bool, ray_packet_width>
aabb::hit_packet_avx2(const ray_packet &rays, interval ray_t) const {
#if defined(__AVX2__)
  std::array<bool, ray_packet_width> hits{};
  const __m256d ray_min = _mm256_set1_pd(ray_t.min);
  const __m256d ray_max = _mm256_set1_pd(ray_t.max);
  const __m256d epsilon = _mm256_set1_pd(1e-12);
  const __m256d sign_mask = _mm256_set1_pd(-0.0);

  for (std::size_t offset = 0; offset < ray_packet_width; offset += 4) {
    __m256d current_min = ray_min;
    __m256d current_max = ray_max;
    __m256d active = _mm256_castsi256_pd(_mm256_set1_epi64x(-1));

    const auto hit_axis = [&](const std::array<double, ray_packet_width> &origins,
                              const std::array<double, ray_packet_width> &directions,
                              const interval &axis_range) {
      const __m256d origin = _mm256_loadu_pd(origins.data() + offset);
      const __m256d direction = _mm256_loadu_pd(directions.data() + offset);
      const __m256d absolute_direction =
          _mm256_andnot_pd(sign_mask, direction);
      const __m256d parallel =
          _mm256_cmp_pd(absolute_direction, epsilon, _CMP_LT_OQ);

      const __m256d t0 = _mm256_div_pd(
          _mm256_sub_pd(_mm256_set1_pd(axis_range.min), origin), direction);
      const __m256d t1 = _mm256_div_pd(
          _mm256_sub_pd(_mm256_set1_pd(axis_range.max), origin), direction);
      const __m256d lower = _mm256_min_pd(t0, t1);
      const __m256d upper = _mm256_max_pd(t0, t1);
      const __m256d next_min = _mm256_max_pd(current_min, lower);
      const __m256d next_max = _mm256_min_pd(current_max, upper);
      current_min = _mm256_blendv_pd(next_min, current_min, parallel);
      current_max = _mm256_blendv_pd(next_max, current_max, parallel);

      const __m256d contained = _mm256_and_pd(
          _mm256_cmp_pd(origin, _mm256_set1_pd(axis_range.min), _CMP_GE_OQ),
          _mm256_cmp_pd(origin, _mm256_set1_pd(axis_range.max), _CMP_LE_OQ));
      const __m256d non_parallel_hit =
          _mm256_cmp_pd(current_max, current_min, _CMP_GT_OQ);
      const __m256d axis_hit =
          _mm256_or_pd(_mm256_and_pd(parallel, contained),
                       _mm256_andnot_pd(parallel, non_parallel_hit));
      active = _mm256_and_pd(active, axis_hit);
    };

    hit_axis(rays.origin_x, rays.direction_x, x);
    hit_axis(rays.origin_y, rays.direction_y, y);
    hit_axis(rays.origin_z, rays.direction_z, z);

    const int mask = _mm256_movemask_pd(active);
    for (std::size_t lane = 0; lane < 4; ++lane) {
      hits[offset + lane] = (mask & (1 << lane)) != 0;
    }
  }
  return hits;
#else
  return hit_packet(rays, ray_t);
#endif
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
