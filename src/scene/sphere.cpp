#include "sphere.h"

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

aabb sphere::bounding_box() const {
  const vec3 radius_vector(radius, radius, radius);
  return aabb(center - radius_vector, center + radius_vector);
}
