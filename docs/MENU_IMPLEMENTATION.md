# Menu System Implementation

This document describes the menu system implementation for the GE-Fangame project.

## Overview

The game now starts with a main menu instead of jumping directly into gameplay. The menu system is designed to be modular and reusable across different scenes.

## Architecture

### Components

1. **UI Menu Component** (`ge-app/include/ge-app/ui/menu.hpp`)
   - Reusable menu widget that can be used in any scene
   - Handles rendering of menu items with selection indicator
   - Manages joystick navigation with configurable thresholds
   - **No dynamic allocation**: Uses user-provided arrays to avoid heap allocation
   - Simple text rendering without width calculation

2. **MenuScene** (`ge-app/include/ge-app/scenes/menu_scene.hpp`)
   - Main menu scene with title, subtitle, and menu options
   - Provides "Start Game", "Options", "Credits", and "Exit" options
   - Demonstrates menu navigation with multiple items

3. **Scene Management** (`ge-app/src/main.cpp`)
   - MainApp now supports switching between different scenes
   - Uses static storage for scenes (no heap allocation)
   - Delegates input events to the current scene

### Input Controls

- **Joystick Y-axis**: Navigate menu items (up/down)
  - Threshold for movement: ±0.5
  - Threshold for center detection: ±0.3
- **Button 1**: Select the highlighted menu item
- **Button 2**: Currently has no function in the menu (reserved for future use)

## Key Features

### Performance Optimizations

1. **Simple Rendering**: Menu items rendered directly without width calculation - just eyeball positioning
2. **Efficient Rendering**: Only renders necessary elements, no redundant calculations
3. **No Dynamic Allocation**: Menu uses pointer to user-provided array, scenes use static storage

### Type Safety

1. **Enum Class**: MenuAction uses `enum class` for strong typing
2. **Proper Type Conversions**: All size_t and int conversions are explicit
3. **Const Correctness**: Methods marked const where appropriate

### Modularity

1. **Reusable Components**: The Menu class can be used in GameScene or other future scenes
2. **Clean Separation**: UI logic is separate from scene logic
3. **Extensible Design**: Easy to add new menu items or create new scenes

## Usage Example

To use the menu component in a different scene:

```cpp
#include "ge-app/ui/menu.hpp"

class MyScene : public Scene {
public:
  MyScene(App &app) : Scene{app} {
    // Initialize menu items in a static array (no dynamic allocation)
    menu_items[0] = {"Option 1", 1};
    menu_items[1] = {"Option 2", 2};
    menu_items[2] = {"Option 3", 3};
    menu_items[3] = {"Option 4", 4};
    menu.set_items(menu_items, 4);
  }

  void tick(float dt) override {
    auto joystick = app.get_joystick_state();
    menu.move_selection(joystick.y);
  }

  void render(Surface &fb) override {
    menu.render(fb, Font::regular_font());
  }

  void on_button_clicked(Button btn) override {
    if (btn == Button::Button1) {
      int selected = menu.get_selected_id();
      // Handle selection
    }
  }

private:
  ui::Menu menu;
  ui::MenuItem menu_items[4];
};
```

## Files Modified

- `ge-app/CMakeLists.txt`: Added new header files for scenes and UI components
- `ge-app/src/main.cpp`: Added scene management for Settings and Credits scenes
- `MENU_IMPLEMENTATION.md`: Updated documentation with new scenes and components

## Files Added

- `ge-app/include/ge-app/ui/menu.hpp`: Reusable menu UI component
- `ge-app/include/ge-app/ui/slider.hpp`: Reusable slider widget for volume controls
- `ge-app/include/ge-app/ui/option_selector.hpp`: Reusable option selector widget
- `ge-app/include/ge-app/scenes/menu_scene.hpp`: Main menu scene implementation
- `ge-app/include/ge-app/scenes/settings_scene.hpp`: Settings scene with volume sliders and button flip
- `ge-app/include/ge-app/scenes/credits_scene.hpp`: Credits scene with placeholder content

## Implemented Scenes

### Settings Scene (`ge-app/include/ge-app/scenes/settings_scene.hpp`)

The Settings scene allows players to configure game options:

**Controls:**
- **Joystick Y-axis**: Navigate between settings items (up/down)
- **Joystick X-axis**: Adjust slider values or toggle options (left/right)
- **Button 1**: Confirm back to menu when "Back to Menu" is selected

**Settings Options:**
1. **Music Volume Slider**: Adjusts background music volume (0-100%)
   - Directly updates the app's master volume in real-time
   - Visual slider with fill bar showing current level

2. **SFX Volume Slider**: Adjusts sound effects volume (0-100%)
   - Separate control for sound effects
   - Visual slider with fill bar showing current level

3. **Button Layout Toggle**: Flip A/B button mapping
   - Options: "Normal (A/B)" or "Flipped (B/A)"
   - Uses arrow navigation to switch between options

4. **Back to Menu**: Returns to main menu

**UI Components Used:**
- `ui::Slider` - Reusable slider widget for volume controls
- `ui::OptionSelector` - Reusable option selector for button flip

### Credits Scene (`ge-app/include/ge-app/scenes/credits_scene.hpp`)

The Credits scene displays game credits with placeholder content:

**Content Sections:**
- **Game Development**: CTB Girls' Dorm
- **Programming**: Placeholder Developer
- **Art & Graphics**: Placeholder Artist
- **Music**: Placeholder Composer
- **Special Thanks**: Placeholder Team

**Controls:**
- **Button 1**: Select back button to return to main menu

### New UI Components

#### Slider Component (`ge-app/include/ge-app/ui/slider.hpp`)

A reusable slider widget for numeric value adjustment:

**Features:**
- Configurable range (min/max values)
- Visual bar with fill indicator
- Joystick X-axis adjustment
- Automatic value clamping
- Volume conversion method (0-255 for audio)

**Usage:**
```cpp
ui::Slider music_slider;
music_slider.set_label("Music Volume");
music_slider.set_range(0.0f, 100.0f);
music_slider.set_value(100.0f);

// In tick():
music_slider.adjust_value(joystick.x, 2.0f);

// Get volume:
u8 volume = music_slider.get_value_as_volume();
```

#### Option Selector Component (`ge-app/include/ge-app/ui/option_selector.hpp`)

A reusable option selector widget for toggling between choices:

**Features:**
- Configurable list of options
- Arrow indicators for navigation
- Joystick X-axis selection
- Support for multiple options (2+)

**Usage:**
```cpp
const char* options[] = {"Normal", "Flipped"};
ui::OptionSelector selector;
selector.set_label("Button Layout");
selector.set_options(options, 2, 0);

// In tick():
selector.adjust_selection(joystick.x);

// Get selection:
u32 index = selector.get_selected_index();
```

## Future Enhancements

Potential improvements for the menu system:

1. **Animations**: Add fade-in/fade-out transitions between scenes
2. **Sound Effects**: Play sounds when navigating or selecting menu items
3. **Pause Menu**: Use the menu component in GameScene for a pause menu
4. **Visual Effects**: Add particle effects or animated backgrounds
5. **Persistent Settings**: Save settings to flash memory on STM32
6. **Additional Credits**: Replace placeholder credits with actual contributors

## Testing

The implementation should be tested to verify:

**Main Menu:**
- [x] Menu appears on startup
- [ ] Joystick navigation works correctly (up/down movement)
- [ ] Button 1 selects the highlighted option
- [ ] "Start Game" transitions to GameScene
- [ ] "Options" transitions to SettingsScene
- [ ] "Credits" transitions to CreditsScene
- [ ] "Exit" closes the application (PC only)
- [ ] Menu renders correctly on both PC and STM32 displays

**Settings Scene:**
- [ ] Music volume slider adjusts with joystick X-axis
- [ ] SFX volume slider adjusts with joystick X-axis
- [ ] Button flip option toggles between Normal/Flipped
- [ ] Visual feedback shows selected item clearly
- [ ] Back button returns to main menu
- [ ] Settings changes persist during session

**Credits Scene:**
- [ ] All placeholder credits display correctly
- [ ] Back button returns to main menu
- [ ] Text is readable and properly aligned

Note: Due to build environment constraints, full testing requires the Nix development environment or manual setup of SDL3 and build tools. The implementation will be tested via CI/CD on push.
