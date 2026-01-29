#include "ge-app/scenes/main.hpp"
#include "ge-app/scenes/dialog.hpp"

namespace ge {
namespace scenes {
RootScene::RootScene(App &app)
    : ContainerScene(app), game_scene(*this), menu_scene(*this),
      dialog_scene(*this) {
  set_scenes(sub_scenes);
  menu_scene.on_enter();
}

void RootScene::start_game() {
  menu_scene.on_exit();
  current_screen = RootSceneScreen::Game;
  game_scene.on_enter();
  game_scene.start_new_game();
}

void RootScene::exit() { app.request_quit(); }

} // namespace scenes
} // namespace ge
