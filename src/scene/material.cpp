#include "material.h"

bool material::scatter(const ray &, const hit_record &, color &, ray &) const {
  return false;
}

lambertian::lambertian(const color &albedo) : albedo(albedo) {}

bool lambertian::scatter(const ray &, const hit_record &rec, color &attenuation,
                         ray &scattered) const {
  auto scatter_direction = rec.normal + random_unit_vector();
  if (scatter_direction.near_zero())
    scatter_direction = rec.normal;
  scattered = ray(rec.p, scatter_direction);
  attenuation = albedo;
  return true;
}

metal::metal(const color &albedo, double fuzz)
    : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

bool metal::scatter(const ray &r_in, const hit_record &rec, color &attenuation,
                    ray &scattered) const {
  auto reflected = unit_vector(reflect(r_in.direction(), rec.normal)) +
                   fuzz * random_unit_vector();
  scattered = ray(rec.p, reflected);
  attenuation = albedo;
  return dot(scattered.direction(), rec.normal) > 0;
}

dielectric::dielectric(double refraction_index)
    : refraction_index(refraction_index) {}

bool dielectric::scatter(const ray &r_in, const hit_record &rec,
                         color &attenuation, ray &scattered) const {
  attenuation = color(1.0, 1.0, 1.0);
  const auto ri = rec.front_face ? 1.0 / refraction_index : refraction_index;
  const auto unit_direction = unit_vector(r_in.direction());
  const auto cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
  const auto sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);
  const bool cannot_refract = ri * sin_theta > 1.0;
  const auto direction =
      cannot_refract || reflectance(cos_theta, ri) > random_double()
          ? reflect(unit_direction, rec.normal)
          : refract(unit_direction, rec.normal, ri);
  scattered = ray(rec.p, direction);
  return true;
}

double dielectric::reflectance(double cosine, double refraction_index) {
  auto r0 = (1 - refraction_index) / (1 + refraction_index);
  r0 *= r0;
  return r0 + (1 - r0) * std::pow(1 - cosine, 5);
}
