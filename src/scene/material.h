#ifndef MATERIAL_H
#define MATERIAL_H
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

#include "hittable.h"

class material {
public:
  virtual ~material() = default;

  virtual bool scatter(const ray &, const hit_record &, color &, ray &) const;
};

class lambertian : public material {
public:
  explicit lambertian(const color &albedo);
  bool scatter(const ray &, const hit_record &, color &, ray &) const override;

private:
  color albedo;
};

class metal : public material {
public:
  metal(const color &albedo, double fuzz);
  bool scatter(const ray &, const hit_record &, color &, ray &) const override;

private:
  color albedo;
  double fuzz;
};

class dielectric : public material {
public:
  explicit dielectric(double refraction_index);
  bool scatter(const ray &, const hit_record &, color &, ray &) const override;

private:
  // Refractive index in vacuum or air, or the ratio of the material's
  // refractive index over the refractive index of the enclosing media
  double refraction_index;

  static double reflectance(double cosine, double refraction_index);
};

#endif
