#include "ge-app/scenes/main.hpp"
#include "ge-app/rng.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/surface.hpp"

namespace ge {

class MainApp : public App {
public:
  MainApp() : root_scene(*this) {}

  void tick(float dt) override {
    App::tick(dt);
    root_scene.tick(dt);

    auto joystick = get_joystick_state();
    root_scene.on_joystick_moved(dt, joystick.x, joystick.y);
  }

  void render(Surface &fb) override {
    App::render(fb);
    auto start = now();
    root_scene.render(fb);
    auto end = now();
    // log("Frame render time: %d ms", static_cast<int>(end - start));
  }

  void on_button_clicked(Button btn) override {
    App::on_button_clicked(btn);
    root_scene.on_button_clicked(btn);
    log("Button %d clicked", static_cast<int>(btn));
  }

  void on_button_held(Button btn) override {
    App::on_button_held(btn);
    root_scene.on_button_held(btn);
  }

  void on_button_finished_hold(Button btn) override {
    App::on_button_finished_hold(btn);
    root_scene.on_button_finished_hold(btn);
  }

private:
  scenes::RootScene root_scene;
};

} // namespace ge

int main() {
  ge::rng::init_seed();
  ge::MainApp app;
  app.loop();
  return 0;
}
