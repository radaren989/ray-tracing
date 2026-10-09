#ifndef CAMERA_H
#define CAMERA_H

#include "../scene/hittable.h"

#include <atomic>
#include <iostream>
#include <vector>

class camera {
public:
  double aspect_ratio = 1.0;
  int image_width = 100;
  int samples_per_pixel = 10;
  int max_depth = 10;
  unsigned int thread_count = 1;
  double vfov = 90;
  point3 lookfrom = point3(0, 0, 0);
  point3 lookat = point3(0, 0, -1);
  vec3 vup = vec3(0, 1, 0);
  double defocus_angle = 0;
  double focus_dist = 10;

  void render(const hittable &world, std::ostream &output = std::cout,
              bool show_progress = true);

private:
  int image_height;
  double pixel_samples_scale;
  point3 center;
  point3 pixel00_loc;
  vec3 pixel_delta_u, pixel_delta_v;
  vec3 u, v, w;
  vec3 defocus_disk_u, defocus_disk_v;

  void initialize();
  void render_rows(const hittable &world, std::vector<color> &pixels,
                   std::atomic<int> &next_row) const;
  ray get_ray(int i, int j) const;
  vec3 sample_square() const;
  point3 defocus_disk_sample() const;
  color ray_color(const ray &r, int depth, const hittable &world) const;
};

#endif
