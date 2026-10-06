#include "color.h"

#include <ostream>

namespace {
double linear_to_gamma(double linear_component) {
  return linear_component > 0 ? std::sqrt(linear_component) : 0;
}
} // namespace

void write_color(std::ostream &out, const color &pixel_color) {
  auto r = linear_to_gamma(pixel_color.x());
  auto g = linear_to_gamma(pixel_color.y());
  auto b = linear_to_gamma(pixel_color.z());

  static const interval intensity(0.000, 0.999);
  const int rbyte = int(256 * intensity.clamp(r));
  const int gbyte = int(256 * intensity.clamp(g));
  const int bbyte = int(256 * intensity.clamp(b));

  out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}
