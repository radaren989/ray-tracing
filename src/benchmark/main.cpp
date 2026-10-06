#include "../scene/scene.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
int parse_argument(const char *value, const char *name) {
  const int parsed = std::atoi(value);
  if (parsed <= 0) {
    std::cerr << name << " must be greater than zero\n";
    std::exit(EXIT_FAILURE);
  }
  return parsed;
}
} // namespace

int main(int argc, char **argv) {
  const int image_width = argc > 1 ? parse_argument(argv[1], "width") : 320;
  const int samples_per_pixel =
      argc > 2 ? parse_argument(argv[2], "samples") : 10;
  const int max_depth = argc > 3 ? parse_argument(argv[3], "depth") : 20;
  const int iterations = argc > 4 ? parse_argument(argv[4], "iterations") : 3;

  std::ofstream sink("/dev/null");
  if (!sink) {
    std::cerr << "could not open benchmark output sink\n";
    return EXIT_FAILURE;
  }

  std::cout << "width,samples_per_pixel,max_depth,iteration,seconds,rays_per_"
               "second\n";
  for (int iteration = 0; iteration < iterations; ++iteration) {
    auto world = make_default_scene();
    auto cam = make_default_camera(image_width, samples_per_pixel, max_depth);

    const auto start = std::chrono::steady_clock::now();
    cam.render(world, sink, false);
    const auto end = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(end - start).count();
    const double image_height =
        static_cast<int>(image_width / cam.aspect_ratio);
    const double rays =
        static_cast<double>(image_width) * image_height * samples_per_pixel;
    std::cout << image_width << ',' << samples_per_pixel << ',' << max_depth
              << ',' << iteration + 1 << ',' << seconds << ',' << rays / seconds
              << '\n';
  }
}
