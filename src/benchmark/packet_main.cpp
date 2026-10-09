#include "aabb.h"
#include "bvh_node.h"
#include "ray_packet.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
constexpr std::size_t packet_count = 1 << 16;
constexpr std::size_t repetitions = 100;
constexpr std::size_t bvh_packet_count = 1 << 14;

const std::array<ray_packet, packet_count> &make_packets() {
  static std::array<ray_packet, packet_count> result = {};
  static bool initialized = false;
  if (!initialized) {
    for (std::size_t packet = 0; packet < packet_count; ++packet) {
      std::array<ray, ray_packet_width> rays;
      for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
        const double x =
            static_cast<double>((packet * ray_packet_width + lane) % 101) *
                0.02 -
            1.0;
        const double y =
            static_cast<double>((packet * ray_packet_width + lane) % 67) *
                0.03 -
            1.0;
        rays[lane] = ray(point3(x, y, 0), vec3(-x * 0.2, -y * 0.2, -1));
      }
      result[packet] = ray_packet(rays);
    }
    initialized = true;
  }
  return result;
}

std::array<ray_packet, bvh_packet_count> make_bvh_packets() {
  std::array<ray_packet, bvh_packet_count> result = {};
  for (std::size_t packet = 0; packet < bvh_packet_count; ++packet) {
    std::array<ray, ray_packet_width> rays;
    for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
      const double x = static_cast<double>(
                           (packet * ray_packet_width + lane) % 101) *
                           0.02 -
                       1.0;
      const double y = static_cast<double>(
                           (packet * ray_packet_width + lane) % 67) *
                           0.03 -
                       1.0;
      rays[lane] = ray(point3(0, 0, 0), vec3(x, y, -1));
    }
    result[packet] = ray_packet(rays);
  }
  return result;
}

std::uint64_t count_bvh_hits(
    const bvh_node &bvh,
    const std::array<ray_packet, bvh_packet_count> &packets) {
  std::uint64_t total = 0;
  for (std::size_t repetition = 0; repetition < repetitions; ++repetition) {
    for (const ray_packet &packet : packets) {
      const auto hits = bvh.hit_packet(packet, interval(0.001, infinity));
      for (bool hit : hits) {
        total += hit;
      }
    }
  }
  return total;
}

std::uint64_t count_scalar_bvh_hits(
    const bvh_node &bvh,
    const std::array<ray_packet, bvh_packet_count> &packets) {
  std::uint64_t total = 0;
  for (std::size_t repetition = 0; repetition < repetitions; ++repetition) {
    for (const ray_packet &packet : packets) {
      for (std::size_t lane = 0; lane < ray_packet_width; ++lane) {
        hit_record record;
        total += bvh.hit(packet.ray_at(lane), interval(0.001, infinity),
                         record);
      }
    }
  }
  return total;
}

std::uint64_t count_hits(const aabb &box,
                         const std::array<ray_packet, packet_count> &packets,
                         bool avx2) {
  std::uint64_t total = 0;
  for (std::size_t repetition = 0; repetition < repetitions; ++repetition) {
    for (const ray_packet &packet : packets) {
      const auto hits = avx2
                            ? box.hit_packet_avx2(packet, interval(0.001, 100))
                            : box.hit_packet(packet, interval(0.001, 100));
      for (bool hit : hits) {
        total += hit;
      }
    }
  }
  return total;
}

double measure(const aabb &box,
               const std::array<ray_packet, packet_count> &packets,
               bool avx2, std::uint64_t &checksum) {
  const auto start = std::chrono::steady_clock::now();
  checksum = count_hits(box, packets, avx2);
  const auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(end - start).count();
}
} // namespace

int main() {
  const aabb box(point3(-1, -1, -5), point3(1, 1, -2));
  const auto &packets = make_packets();
  std::uint64_t scalar_checksum = 0;
  std::uint64_t avx2_checksum = 0;
  const double scalar_seconds =
      measure(box, packets, false, scalar_checksum);
  const double avx2_seconds = measure(box, packets, true, avx2_checksum);

  std::cout << "implementation,seconds,packets_per_second,hit_checksum\n";
  std::cout << "scalar," << scalar_seconds << ','
            << (packet_count * repetitions) / scalar_seconds << ','
            << scalar_checksum << '\n';
  std::cout << "avx2," << avx2_seconds << ','
            << (packet_count * repetitions) / avx2_seconds << ','
            << avx2_checksum << '\n';

  hittable_list objects;
  const auto material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
  for (int index = 0; index < 32; ++index) {
    const double x = (index % 8) * 1.5 - 5.25;
    const double y = (index / 8) * 1.5 - 2.25;
    objects.add(make_shared<sphere>(point3(x, y, -8), 0.5, material));
  }
  bvh_node bvh(objects.objects);
  const auto bvh_packets = make_bvh_packets();
  const auto scalar_bvh_start = std::chrono::steady_clock::now();
  const std::uint64_t scalar_bvh_checksum =
      count_scalar_bvh_hits(bvh, bvh_packets);
  const auto scalar_bvh_end = std::chrono::steady_clock::now();
  const double scalar_bvh_seconds =
      std::chrono::duration<double>(scalar_bvh_end - scalar_bvh_start).count();

  const auto start = std::chrono::steady_clock::now();
  const std::uint64_t bvh_checksum = count_bvh_hits(bvh, bvh_packets);
  const auto end = std::chrono::steady_clock::now();
  const double bvh_seconds =
      std::chrono::duration<double>(end - start).count();
  std::cout << "bvh_scalar_seconds," << scalar_bvh_seconds
            << ",hit_checksum," << scalar_bvh_checksum << '\n';
  std::cout << "bvh_packet_seconds," << bvh_seconds
            << ",hit_checksum," << bvh_checksum
            << ",speedup," << scalar_bvh_seconds / bvh_seconds << '\n';

  return scalar_checksum == avx2_checksum &&
                 scalar_bvh_checksum == bvh_checksum
             ? 0
             : 1;
}
