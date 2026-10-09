#include "../scene/scene.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

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
  const unsigned int thread_count =
      argc > 5 ? static_cast<unsigned int>(parse_argument(argv[5], "threads"))
               : 1;

  std::ofstream sink("/dev/null");
  if (!sink) {
    std::cerr << "could not open benchmark output sink\n";
    return EXIT_FAILURE;
  }

  std::cout << "width,samples_per_pixel,max_depth,threads,iteration,seconds,"
               "rays_per_second\n";
  const bool use_linear_scene = std::getenv("RAY_TRACING_SCENE") != nullptr &&
                                std::string(std::getenv("RAY_TRACING_SCENE")) ==
                                    "linear";
  for (int iteration = 0; iteration < iterations; ++iteration) {
    shared_ptr<hittable> world;
    if (use_linear_scene) {
      world = make_shared<hittable_list>(make_default_scene());
    } else {
      world = make_shared<bvh_node>(make_bvh_scene());
    }
    auto cam = make_default_camera(image_width, samples_per_pixel, max_depth,
                                   thread_count);

    const auto start = std::chrono::steady_clock::now();
    cam.render(*world, sink, false);
    const auto end = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(end - start).count();
    const double image_height =
        static_cast<int>(image_width / cam.aspect_ratio);
    const double rays =
        static_cast<double>(image_width) * image_height * samples_per_pixel;
    std::cout << image_width << ',' << samples_per_pixel << ',' << max_depth
              << ',' << thread_count << ',' << iteration + 1 << ',' << seconds
              << ',' << rays / seconds << '\n';
  }
}
