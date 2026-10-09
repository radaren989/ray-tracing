#include "bvh_node.h"

#include <algorithm>
#include <stdexcept>

bvh_node::bvh_node(std::vector<shared_ptr<hittable>> objects)
    : bvh_node(objects, 0, objects.size()) {}

bvh_node::bvh_node(std::vector<shared_ptr<hittable>> &objects, size_t start,
                   size_t end) {
  if (start >= end) {
    throw std::invalid_argument("cannot build a BVH node from an empty range");
  }

  box = objects[start]->bounding_box();
  for (size_t i = start + 1; i < end; ++i) {
    box = surrounding_box(box, objects[i]->bounding_box());
  }

  const auto x_size = box.x.size();
  const auto y_size = box.y.size();
  const auto z_size = box.z.size();
  int axis = 0;
  if (y_size > x_size && y_size >= z_size) {
    axis = 1;
  } else if (z_size > x_size && z_size > y_size) {
    axis = 2;
  }

  std::sort(objects.begin() + start, objects.begin() + end,
            [axis](const shared_ptr<hittable> &first,
                   const shared_ptr<hittable> &second) {
              const auto first_box = first->bounding_box();
              const auto second_box = second->bounding_box();
              const auto &first_axis = first_box.axis_interval(axis);
              const auto &second_axis = second_box.axis_interval(axis);
              return first_axis.min + first_axis.max <
                     second_axis.min + second_axis.max;
            });

  const auto object_count = end - start;
  if (object_count == 1) {
    left = objects[start];
  } else if (object_count == 2) {
    left = objects[start];
    right = objects[start + 1];
  } else {
    const auto middle = start + object_count / 2;
    left = shared_ptr<hittable>(new bvh_node(objects, start, middle));
    right = shared_ptr<hittable>(new bvh_node(objects, middle, end));
  }
}

bool bvh_node::hit(const ray &r, interval ray_t, hit_record &rec) const {
  if (!box.hit(r, ray_t)) {
    return false;
  }

  const bool hit_left = left->hit(r, ray_t, rec);
  if (!right) {
    return hit_left;
  }

  if (hit_left) {
    ray_t.max = rec.t;
  }

  const bool hit_right = right->hit(r, ray_t, rec);
  return hit_left || hit_right;
}

aabb bvh_node::bounding_box() const {
  return box;
}
