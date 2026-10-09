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

std::array<bool, ray_packet_width>
bvh_node::hit_packet(const ray_packet &rays, interval ray_t) const {
  std::array<bool, ray_packet_width> active{};
  active.fill(true);
  return hit_packet_active(rays, ray_t, active);
}

std::array<bool, ray_packet_width>
bvh_node::hit_packet_active(
    const ray_packet &rays, interval ray_t,
    const std::array<bool, ray_packet_width> &active) const {
  std::array<bool, ray_packet_width> hits{};
  const auto box_hits = box.hit_packet_avx2(rays, ray_t);
  std::array<bool, ray_packet_width> node_hits{};
  bool any_node_hit = false;
  for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
    node_hits[lane] = active[lane] && box_hits[lane];
    any_node_hit = any_node_hit || node_hits[lane];
  }
  if (!any_node_hit) {
    return hits;
  }

  const auto visit_child = [&](const shared_ptr<hittable> &child) {
    if (!child) {
      return;
    }

    const auto child_box_hits =
        child->bounding_box().hit_packet_avx2(rays, ray_t);
    std::array<bool, ray_packet_width> child_active{};
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      child_active[lane] = node_hits[lane] && child_box_hits[lane];
    }

    bool any_child_active = false;
    for (bool lane_active : child_active) {
      any_child_active = any_child_active || lane_active;
    }
    if (!any_child_active) {
      return;
    }

    const auto child_hits =
        child->hit_packet_active(rays, ray_t, child_active);

    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      hits[lane] = hits[lane] || child_hits[lane];
    }
  };

  visit_child(left);
  visit_child(right);
  return hits;
}

std::array<bool, ray_packet_width>
bvh_node::hit_packet_records(
    const ray_packet &rays, interval ray_t, packet_hit_records &records,
    const std::array<bool, ray_packet_width> &active) const {
  std::array<bool, ray_packet_width> hits{};
  const auto box_hits = box.hit_packet_avx2(rays, ray_t);
  std::array<bool, ray_packet_width> node_active{};
  for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
    node_active[lane] = active[lane] && box_hits[lane];
  }

  const auto visit_child = [&](const shared_ptr<hittable> &child) {
    if (!child) {
      return;
    }

    interval child_interval = ray_t;
    std::array<bool, ray_packet_width> child_active{};
    const auto child_box_hits =
        child->bounding_box().hit_packet_avx2(rays, child_interval);
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      child_active[lane] = node_active[lane] && child_box_hits[lane];
      if (hits[lane]) {
        child_interval.max = std::min(child_interval.max, records[lane].t);
      }
    }

    packet_hit_records child_records{};
    const auto child_hits = child->hit_packet_records(
        rays, child_interval, child_records, child_active);
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      if (child_hits[lane] && (!hits[lane] || child_records[lane].t <
                                                records[lane].t)) {
        hits[lane] = true;
        records[lane] = child_records[lane];
      }
    }
  };

  visit_child(left);
  visit_child(right);
  return hits;
}

aabb bvh_node::bounding_box() const {
  return box;
}
