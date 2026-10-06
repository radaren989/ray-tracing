#include "../scene/scene.h"

int main() {
  auto world = make_default_scene();
  auto cam = make_default_camera(1200, 10, 20);
  cam.render(world);
}
