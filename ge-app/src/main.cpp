#include "ge-app/scenes/game_scene.hpp"
#include "ge-app/scenes/gameover_scene.hpp"
#include "ge-app/scenes/menu_scene.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/surface.hpp"

#include <cassert>

namespace ge {

enum class SceneType { Menu, Game, GameOver };

class MainApp : public ge::App {
public:
  MainApp()
      : ge::App(), menu_scene_impl(*this), game_scene{*this},
        gameover_scene{*this} {
    switch_to_menu();
  }

  void tick(float dt) override {
    App::tick(dt);
    if (current_scene) {
      current_scene->tick(dt);
    }

    // Check for game over
    if (current_scene_type == SceneType::Game && game_scene.is_game_over()) {
      switch_to_gameover();
    }

    // Check for scene transitions from game over
    if (current_scene_type == SceneType::GameOver) {
      if (gameover_scene.wants_restart()) {
        game_scene.reset();
        gameover_scene.reset();
        switch_to_game();
      } else if (gameover_scene.wants_menu()) {
        gameover_scene.reset();
        switch_to_menu();
      }
    }
  }

  void render(Surface &fb) override {
    if (current_scene) {
      current_scene->render(fb);
    }
  }

  void on_button_clicked(Button btn) override {
    if (current_scene) {
      current_scene->on_button_clicked(btn);
    }
  }

  void on_button_held(Button btn) override {
    if (current_scene) {
      current_scene->on_button_held(btn);
    }
  }

  void on_button_finished_hold(Button btn) override {
    if (current_scene) {
      current_scene->on_button_finished_hold(btn);
    }
  }

  void switch_to_menu() {
    current_scene_type = SceneType::Menu;
    current_scene = &menu_scene_impl;
  }

  void switch_to_game() {
    current_scene_type = SceneType::Game;
    current_scene = &game_scene;
  }

  void switch_to_gameover() {
    current_scene_type = SceneType::GameOver;
    current_scene = &gameover_scene;
  }

private:
  class MenuSceneImpl : public MenuScene {
  public:
    MenuSceneImpl(MainApp &app) : MenuScene(app), main_app(app) {}

    void on_menu_action(MenuAction action) override {
      if (action == MenuAction::StartGame) {
        main_app.switch_to_game();
      } else if (action == MenuAction::Options) {
        // Placeholder for options menu
        main_app.log("Options selected (not implemented yet)");
      } else if (action == MenuAction::Credits) {
        // Placeholder for credits screen
        main_app.log("Credits selected (not implemented yet)");
      } else if (action == MenuAction::ExitGame) {
        main_app.request_quit();
      }
    }

  private:
    MainApp &main_app;
  };

  SceneType current_scene_type = SceneType::Menu;
  Scene *current_scene = nullptr;
  MenuSceneImpl menu_scene_impl;
  GameScene game_scene;
  GameOverScene gameover_scene;
};
} // namespace ge

int main() {
  ge::MainApp app;
  app.loop();
  return 0;
}
