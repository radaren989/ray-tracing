#include "sphere.h"

#include <cmath>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

sphere::sphere(const point3 &center, double radius, shared_ptr<material> mat)
    : center(center), radius(std::fmax(0, radius)), mat(std::move(mat)) {}

bool sphere::hit(const ray &r, interval ray_t, hit_record &rec) const {
  const vec3 oc = center - r.origin();
  const auto a = r.direction().length_squared();
  const auto h = dot(r.direction(), oc);
  const auto c = oc.length_squared() - radius * radius;
  const auto discriminant = h * h - a * c;
  if (discriminant < 0)
    return false;

  const auto sqrtd = std::sqrt(discriminant);
  auto root = (h - sqrtd) / a;
  if (!ray_t.surrounds(root)) {
    root = (h + sqrtd) / a;
    if (!ray_t.surrounds(root))
      return false;
  }

  rec.t = root;
  rec.p = r.at(rec.t);
  rec.set_face_normal(r, (rec.p - center) / radius);
  rec.mat = mat;
  return true;
}

std::array<bool, ray_packet_width>
sphere::hit_packet_active(
    const ray_packet &rays, interval ray_t,
    const std::array<bool, ray_packet_width> &active) const {
#if defined(__AVX2__)
  std::array<bool, ray_packet_width> hits{};
  const __m256d center_x = _mm256_set1_pd(center.x());
  const __m256d center_y = _mm256_set1_pd(center.y());
  const __m256d center_z = _mm256_set1_pd(center.z());
  const __m256d radius_squared = _mm256_set1_pd(radius * radius);
  const __m256d ray_min = _mm256_set1_pd(ray_t.min);
  const __m256d ray_max = _mm256_set1_pd(ray_t.max);

  for (std::size_t offset = 0; offset < ray_packet_width; offset += 4) {
    const __m256d origin_x = _mm256_loadu_pd(rays.origin_x.data() + offset);
    const __m256d origin_y = _mm256_loadu_pd(rays.origin_y.data() + offset);
    const __m256d origin_z = _mm256_loadu_pd(rays.origin_z.data() + offset);
    const __m256d direction_x =
        _mm256_loadu_pd(rays.direction_x.data() + offset);
    const __m256d direction_y =
        _mm256_loadu_pd(rays.direction_y.data() + offset);
    const __m256d direction_z =
        _mm256_loadu_pd(rays.direction_z.data() + offset);

    const __m256d oc_x = _mm256_sub_pd(center_x, origin_x);
    const __m256d oc_y = _mm256_sub_pd(center_y, origin_y);
    const __m256d oc_z = _mm256_sub_pd(center_z, origin_z);
    const __m256d a = _mm256_add_pd(
        _mm256_add_pd(_mm256_mul_pd(direction_x, direction_x),
                      _mm256_mul_pd(direction_y, direction_y)),
        _mm256_mul_pd(direction_z, direction_z));
    const __m256d h = _mm256_add_pd(
        _mm256_add_pd(_mm256_mul_pd(direction_x, oc_x),
                      _mm256_mul_pd(direction_y, oc_y)),
        _mm256_mul_pd(direction_z, oc_z));
    const __m256d c = _mm256_sub_pd(
        _mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(oc_x, oc_x),
                                    _mm256_mul_pd(oc_y, oc_y)),
                      _mm256_mul_pd(oc_z, oc_z)),
        radius_squared);
    const __m256d discriminant = _mm256_sub_pd(
        _mm256_mul_pd(h, h), _mm256_mul_pd(a, c));
    const __m256d valid_discriminant =
        _mm256_cmp_pd(discriminant, _mm256_setzero_pd(), _CMP_GE_OQ);
    const __m256d square_root = _mm256_sqrt_pd(
        _mm256_max_pd(discriminant, _mm256_setzero_pd()));
    const __m256d first_root =
        _mm256_div_pd(_mm256_sub_pd(h, square_root), a);
    const __m256d second_root =
        _mm256_div_pd(_mm256_add_pd(h, square_root), a);
    const __m256d first_valid = _mm256_and_pd(
        _mm256_cmp_pd(first_root, ray_min, _CMP_GT_OQ),
        _mm256_cmp_pd(first_root, ray_max, _CMP_LT_OQ));
    const __m256d second_valid = _mm256_and_pd(
        _mm256_cmp_pd(second_root, ray_min, _CMP_GT_OQ),
        _mm256_cmp_pd(second_root, ray_max, _CMP_LT_OQ));
    const __m256d lane_hits =
        _mm256_and_pd(valid_discriminant,
                      _mm256_or_pd(first_valid, second_valid));
    const int mask = _mm256_movemask_pd(lane_hits);
    for (std::size_t lane = 0; lane < 4; ++lane) {
      hits[offset + lane] =
          active[offset + lane] && (mask & (1 << lane)) != 0;
    }
  }
  return hits;
#else
  return hittable::hit_packet_active(rays, ray_t, active);
#endif
}

aabb sphere::bounding_box() const {
  const vec3 radius_vector(radius, radius, radius);
  return aabb(center - radius_vector, center + radius_vector);
}
