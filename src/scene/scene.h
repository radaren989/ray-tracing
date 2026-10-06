#ifndef SCENE_H
#define SCENE_H

#include "../renderer/camera.h"
#include "hittable_list.h"

hittable_list make_default_scene();
camera make_default_camera(int image_width, int samples_per_pixel,
                           int max_depth);

#endif
