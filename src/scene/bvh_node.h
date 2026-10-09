#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "hittable.h"

#include <array>
#include <vector>

class bvh_node : public hittable {
public:
  explicit bvh_node(std::vector<shared_ptr<hittable>> objects);

  bool hit(const ray &r, interval ray_t, hit_record &rec) const override;
  std::array<bool, ray_packet_width>
  hit_packet(const ray_packet &rays, interval ray_t) const override;
  std::array<bool, ray_packet_width>
  hit_packet_records(const ray_packet &rays, interval ray_t,
                     packet_hit_records &records,
                     const std::array<bool, ray_packet_width> &active) const
      override;
  aabb bounding_box() const override;

private:
  bvh_node(std::vector<shared_ptr<hittable>> &objects, size_t start,
           size_t end);
  std::array<bool, ray_packet_width>
  hit_packet_active(const ray_packet &rays, interval ray_t,
                    const std::array<bool, ray_packet_width> &active) const
      override;

  shared_ptr<hittable> left;
  shared_ptr<hittable> right;
  aabb box;
};

#endif
