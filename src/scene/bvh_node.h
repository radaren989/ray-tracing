#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "hittable.h"

#include <vector>

class bvh_node : public hittable {
public:
  explicit bvh_node(std::vector<shared_ptr<hittable>> objects);

  bool hit(const ray &r, interval ray_t, hit_record &rec) const override;
  aabb bounding_box() const override;

private:
  bvh_node(std::vector<shared_ptr<hittable>> &objects, size_t start,
           size_t end);

  shared_ptr<hittable> left;
  shared_ptr<hittable> right;
  aabb box;
};

#endif
