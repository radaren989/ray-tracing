#include "camera.h"

#include "../scene/material.h"

#include <algorithm>
#include <thread>

void camera::render_rows(const hittable &world, std::vector<color> &pixels,
                         std::atomic<int> &next_row) const {
  while (true) {
    const int row = next_row.fetch_add(1, std::memory_order_relaxed);
    if (row >= image_height) {
      return;
    }

    for (int i = 0; i < image_width; ++i) {
      color pixel_color(0, 0, 0);
      for (int sample = 0; sample < samples_per_pixel; ++sample) {
        pixel_color += ray_color(get_ray(i, row), max_depth, world);
      }
      pixels[row * image_width + i] = pixel_samples_scale * pixel_color;
    }
  }
}

void camera::render(const hittable &world, std::ostream &output,
                    bool show_progress) {
  initialize();
  std::vector<color> pixels(image_width * image_height);

  std::atomic<int> next_row{0};

  const unsigned int thread_count = std::min(
      static_cast<unsigned int>(image_height), std::max(1u, this->thread_count));

  std::vector<std::thread> workers;
  workers.reserve(thread_count);

  for (unsigned int thread_id = 0; thread_id < thread_count; ++thread_id) {
    workers.emplace_back(&camera::render_rows, this, std::cref(world),
                         std::ref(pixels), std::ref(next_row));
  }

  for (auto &worker : workers) {
    worker.join();
  }

  output << "P3\n" << image_width << ' ' << image_height << "\n255\n";
  for (const auto &pixel : pixels) {
    write_color(output, pixel);
  }

  if (show_progress)
    std::clog << "\rDone.                 \n";
}

void camera::initialize() {
  image_height = std::max(1, int(image_width / aspect_ratio));
  pixel_samples_scale = 1.0 / samples_per_pixel;
  center = lookfrom;
  const auto viewport_height =
      2 * std::tan(degrees_to_radians(vfov) / 2) * focus_dist;
  const auto viewport_width =
      viewport_height * double(image_width) / image_height;
  w = unit_vector(lookfrom - lookat);
  u = unit_vector(cross(vup, w));
  v = cross(w, u);
  const auto viewport_u = viewport_width * u;
  const auto viewport_v = viewport_height * -v;
  pixel_delta_u = viewport_u / image_width;
  pixel_delta_v = viewport_v / image_height;
  pixel00_loc = center - focus_dist * w - viewport_u / 2 - viewport_v / 2 +
                0.5 * (pixel_delta_u + pixel_delta_v);
  const auto defocus_radius =
      focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
  defocus_disk_u = u * defocus_radius;
  defocus_disk_v = v * defocus_radius;
}

ray camera::get_ray(int i, int j) const {
  const auto offset = sample_square();
  const auto pixel_sample = pixel00_loc + (i + offset.x()) * pixel_delta_u +
                            (j + offset.y()) * pixel_delta_v;
  const auto ray_origin = defocus_angle <= 0 ? center : defocus_disk_sample();
  return ray(ray_origin, pixel_sample - ray_origin);
}

vec3 camera::sample_square() const {
  return {random_double() - 0.5, random_double() - 0.5, 0};
}

point3 camera::defocus_disk_sample() const {
  const auto p = random_in_unit_disk();
  return center + p[0] * defocus_disk_u + p[1] * defocus_disk_v;
}

color camera::ray_color(const ray &r, int depth, const hittable &world) const {
  if (depth <= 0)
    return {0, 0, 0};
  hit_record rec;
  if (world.hit(r, interval(0.001, infinity), rec)) {
    ray scattered;
    color attenuation;
    if (rec.mat->scatter(r, rec, attenuation, scattered)) {
      return attenuation * ray_color(scattered, depth - 1, world);
    }
    return {0, 0, 0};
  }
  const auto unit_direction = unit_vector(r.direction());
  const auto a = 0.5 * (unit_direction.y() + 1.0);
  return (1.0 - a) * color(1, 1, 1) + a * color(0.5, 0.7, 1);
}
