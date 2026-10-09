#include "scene.h"

#include "material.h"
#include "sphere.h"

hittable_list make_default_scene() {
  hittable_list world;

  auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
  world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

  for (int a = -11; a < 11; a++) {
    for (int b = -11; b < 11; b++) {
      auto choose_mat = random_double();
      point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

      if ((center - point3(4, 0.2, 0)).length() > 0.9) {
        shared_ptr<material> sphere_material;

        if (choose_mat < 0.8) {
          sphere_material =
              make_shared<lambertian>(color::random() * color::random());
        } else if (choose_mat < 0.95) {
          sphere_material =
              make_shared<metal>(color::random(0.5, 1), random_double(0, 0.5));
        } else {
          sphere_material = make_shared<dielectric>(1.5);
        }
        world.add(make_shared<sphere>(center, 0.2, sphere_material));
      }
    }
  }

  world.add(
      make_shared<sphere>(point3(0, 1, 0), 1.0, make_shared<dielectric>(1.5)));
  world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0,
                                make_shared<lambertian>(color(0.4, 0.2, 0.1))));
  world.add(make_shared<sphere>(point3(4, 1, 0), 1.0,
                                make_shared<metal>(color(0.7, 0.6, 0.5), 0.0)));

  return world;
}

camera make_default_camera(int image_width, int samples_per_pixel,
                           int max_depth) {
  camera cam;
  cam.aspect_ratio = 16.0 / 9.0;
  cam.image_width = image_width;
  cam.samples_per_pixel = samples_per_pixel;
  cam.max_depth = max_depth;
  cam.vfov = 20;
  cam.lookfrom = point3(13, 2, 3);
  cam.lookat = point3(0, 0, 0);
  cam.vup = vec3(0, 1, 0);
  cam.defocus_angle = 0.6;
  cam.focus_dist = 10.0;
  return cam;
}

bvh_node make_bvh_scene() {
  auto world = make_default_scene();
  return bvh_node(world.objects);
}
