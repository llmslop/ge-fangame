#pragma once

#include "ge-app/font.hpp"
#include "ge-app/scenes/scene.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/gpu.hpp"
#include "ge-hal/surface.hpp"

namespace ge {
class GameOverScene : public Scene {
public:
  GameOverScene(App &app) : Scene{app} {}

  void render(Surface &fb_region) override {
    // Fill screen with dark red/black color
    hal::gpu::fill(fb_region, 0x1800); // Dark red

    // Display "GAME OVER" text
    const char *game_over_text = "GAME OVER";
    const char *restart_text = "Press Button 1 to restart";
    const char *menu_text = "Press Button 2 for menu";

    auto &font = Font::bold_font();

    // Center the text
    u32 center_x = fb_region.get_width() / 2;
    u32 center_y = fb_region.get_height() / 2;

    // Render "GAME OVER" in red
    font.render_colored(game_over_text, -1, fb_region, center_x - 50,
                        center_y - 30, 0xF800); // Red

    // Render instructions in white
    Font::regular_font().render_colored(restart_text, -1, fb_region,
                                        center_x - 80, center_y + 10, 0xFFFF);
    Font::regular_font().render_colored(menu_text, -1, fb_region, center_x - 80,
                                        center_y + 30, 0xFFFF);
  }

  void on_button_clicked(Button btn) override {
    if (btn == Button::Button1) {
      // Signal to restart game
      should_restart = true;
    } else if (btn == Button::Button2) {
      // Signal to go to menu
      should_go_to_menu = true;
    }
  }

  bool wants_restart() const { return should_restart; }
  bool wants_menu() const { return should_go_to_menu; }
  void reset() {
    should_restart = false;
    should_go_to_menu = false;
  }

private:
  bool should_restart = false;
  bool should_go_to_menu = false;
};
} // namespace ge
