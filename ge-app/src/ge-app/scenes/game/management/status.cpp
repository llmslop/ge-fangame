#include "ge-app/scenes/game/management/status.hpp"

#include "ge-app/font.hpp"
#include "ge-app/game/clock.hpp"
#include "ge-app/game/inventory.hpp"
#include "ge-app/game/mode_indicator.hpp"
#include "ge-app/game/player_stats.hpp"
#include "ge-app/gfx/shape_utils.hpp"
#include "ge-app/scenes/game/management/main.hpp"
#include "ge-hal/app.hpp"
#include "ge-hal/gpu.hpp"

#include <cstdio>

namespace ge {
namespace scenes {
namespace game {
namespace mgmt {

StatusScene::StatusScene(ManagementUIScene &parent)
    : Scene(parent.get_app()), parent(parent) {}

void StatusScene::render(Surface &fb_region) {
  // Clear screen with dark background
  hal::gpu::fill(fb_region, 0x0000);

  const auto &font = Font::regular_font();
  u32 y_pos = 10;
  const u32 line_height = font.line_height() + 4;

  // Title
  font.render_colored("=== Player Status ===", -1, fb_region, 10, y_pos,
                      0xFFFF);
  y_pos += line_height + 10;

  // Game Time
  auto &clock = parent.get_clock();
  auto time_str = clock.get_display_string(app);
  font.render_colored(time_str, -1, fb_region, 10, y_pos, 0xF7BE);
  y_pos += line_height;

  // Current Mode
  char mode_buf[32];
  const char *mode_name = "";
  switch (parent.get_current_mode()) {
  case GameMode::Steering:
    mode_name = "Steering";
    break;
  case GameMode::Fishing:
    mode_name = "Fishing";
    break;
  case GameMode::Management:
    mode_name = "Management";
    break;
  }
  snprintf(mode_buf, sizeof(mode_buf), "Current Mode: %s", mode_name);
  font.render_colored(mode_buf, -1, fb_region, 10, y_pos, 0xF7BE);
  y_pos += line_height + 8;

  // Inventory Stats
  font.render_colored("Inventory:", -1, fb_region, 10, y_pos, 0xFFFF);
  y_pos += line_height;

  auto &inventory = parent.get_inventory();
  char inv_buf[64];
  snprintf(inv_buf, sizeof(inv_buf), "  Total Fish: %u / %u",
           inventory.get_item_count(), Inventory::MAX_ITEMS);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0x07FF);
  y_pos += line_height;

  // Fish by rarity
  u32 common_count = inventory.get_count_by_rarity(FishRarity::Common);
  u32 uncommon_count = inventory.get_count_by_rarity(FishRarity::Uncommon);
  u32 rare_count = inventory.get_count_by_rarity(FishRarity::Rare);
  u32 legendary_count = inventory.get_count_by_rarity(FishRarity::Legendary);

  snprintf(inv_buf, sizeof(inv_buf), "  Common: %u", common_count);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0xFFFF);
  y_pos += line_height;

  snprintf(inv_buf, sizeof(inv_buf), "  Uncommon: %u", uncommon_count);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0x07E0);
  y_pos += line_height;

  snprintf(inv_buf, sizeof(inv_buf), "  Rare: %u", rare_count);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0x001F);
  y_pos += line_height;

  snprintf(inv_buf, sizeof(inv_buf), "  Legendary: %u", legendary_count);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0xF81F);
  y_pos += line_height + 10;

  // Status bars
  font.render_colored("Status:", -1, fb_region, 10, y_pos, 0xFFFF);
  y_pos += line_height;

  auto &player_stats = parent.get_player_stats();

  // Ship HP
  u32 ship_hp = player_stats.get_ship_hp();
  u32 max_ship_hp = player_stats.get_max_ship_hp();
  float ship_hp_percent =
      (static_cast<float>(ship_hp) / static_cast<float>(max_ship_hp)) * 100.0f;
  snprintf(inv_buf, sizeof(inv_buf), "  Ship HP: %u/%u", ship_hp, max_ship_hp);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0xF800);
  y_pos += line_height;
  draw_status_bar(fb_region.subsurface(20, y_pos, 100, 8), ship_hp_percent,
                  0xF800);
  y_pos += 12;

  // Food
  float food_percent = player_stats.get_food_percent();
  snprintf(inv_buf, sizeof(inv_buf), "  Food: %.0f%%", food_percent);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0xFBE0);
  y_pos += line_height;

  draw_status_bar(fb_region.subsurface(20, y_pos, 100, 8), food_percent,
                  0xFBE0);
  y_pos += 12;

  // Stamina
  float stamina_percent = player_stats.get_stamina_percent();
  snprintf(inv_buf, sizeof(inv_buf), "  Stamina: %.0f%%", stamina_percent);
  font.render_colored(inv_buf, -1, fb_region, 10, y_pos, 0x07FF);
  y_pos += line_height;

  draw_status_bar(fb_region.subsurface(20, y_pos, 100, 8), stamina_percent,
                  0x07FF);
  y_pos += 12;

  // Instructions
  font.render_colored("Press B to return", -1, fb_region, 10,
                      fb_region.get_height() - line_height - 10, 0x7BEF);
}

bool StatusScene::on_button_clicked(Button btn) {
  if (btn == Button::Button2) {
    on_back_action();
    return true;
  }
  return Scene::on_button_clicked(btn);
}

void StatusScene::draw_status_bar(const Surface &region, float percent,
                                  u16 color) {
  static constexpr u32 BORDER_WIDTH = 1;
  draw_rect(region, 0xFFFF, BORDER_WIDTH);
  u32 fill_width = static_cast<u32>((region.get_width() - BORDER_WIDTH * 2) *
                                    (percent / 100.0f));
  hal::gpu::fill(region.subsurface(BORDER_WIDTH, BORDER_WIDTH, fill_width,
                                   region.get_height() - 2 * BORDER_WIDTH),
                 color);
}

void StatusScene::on_back_action() { parent.back_to_menu(); }

bool StatusScene::is_active() const {
  return parent.is_screen_active(ManagementUIScreen::Status);
}
} // namespace mgmt
} // namespace game
} // namespace scenes
} // namespace ge
