# Scene Flow Diagram

This document illustrates the navigation flow between all scenes in the game.

## Scene Navigation Map

```
┌─────────────────────────────────────────────────────────────────┐
│                         Application Start                        │
└──────────────────────────────────┬──────────────────────────────┘
                                   │
                                   ▼
                    ┌──────────────────────────┐
                    │      Main Menu Scene     │
                    │                          │
                    │  • Start Game            │
                    │  • Options     ─────┐    │
                    │  • Credits     ───┐ │    │
                    │  • Exit           │ │    │
                    └───────┬───────────┼─┼────┘
                            │           │ │
           ┌────────────────┘           │ │
           │                            │ │
           ▼                            │ │
┌──────────────────────┐                │ │
│   Game Scene         │                │ │
│                      │                │ │
│  (Gameplay happens   │                │ │
│   here)              │                │ │
│                      │                │ │
└──────────────────────┘                │ │
                                        │ │
                                        │ │
              ┌─────────────────────────┘ │
              │                           │
              ▼                           ▼
   ┌──────────────────────┐    ┌──────────────────────┐
   │  Settings Scene      │    │   Credits Scene      │
   │                      │    │                      │
   │  • Music Slider ◄─►  │    │  • Game Dev          │
   │  • SFX Slider   ◄─►  │    │  • Programming       │
   │  • Button Flip  ◄─►  │    │  • Art & Graphics    │
   │  • Back to Menu  ────┼────┤  • Music             │
   └──────────────────────┘    │  • Special Thanks    │
              │                │  • Back to Menu  ────┤
              │                └──────────────────────┘
              │                           │
              └───────────┬───────────────┘
                          │
                          ▼
                    Back to Main Menu
```

## Scene Details

### Main Menu Scene
**File:** `ge-app/include/ge-app/scenes/menu_scene.hpp`

**Actions:**
- `StartGame` → Switches to Game Scene
- `Options` → Switches to Settings Scene
- `Credits` → Switches to Credits Scene
- `ExitGame` → Quits application (PC only)

**Controls:**
- Joystick Y: Navigate menu items
- Button 1: Select item

---

### Settings Scene
**File:** `ge-app/include/ge-app/scenes/settings_scene.hpp`

**Items:**
1. **Music Volume Slider** (0-100%)
   - Joystick X: Adjust volume
   - Real-time audio update

2. **SFX Volume Slider** (0-100%)
   - Joystick X: Adjust volume
   - (UI ready, audio system integration pending)

3. **Button Flip Toggle**
   - Joystick X: Switch between Normal/Flipped
   - (UI ready, input system integration pending)

4. **Back to Menu**
   - Button 1: Return to Main Menu

**Controls:**
- Joystick Y: Navigate settings items
- Joystick X: Adjust selected item
- Button 1: Confirm back (when on Back option)

---

### Credits Scene
**File:** `ge-app/include/ge-app/scenes/credits_scene.hpp`

**Content:**
- Game Development: CTB Girls' Dorm
- Programming: Placeholder Developer
- Art & Graphics: Placeholder Artist
- Music: Placeholder Composer
- Special Thanks: Placeholder Team

**Controls:**
- Button 1: Return to Main Menu

---

### Game Scene
**File:** `ge-app/include/ge-app/scenes/game_scene.hpp`

**Description:**
- Main gameplay (existing implementation)
- Not modified in this PR

---

## UI Components

### Menu Component
**File:** `ge-app/include/ge-app/ui/menu.hpp`

**Usage:** Main Menu, Credits (back button)

**Features:**
- Vertical list of items
- Selection indicator
- Joystick navigation

---

### Slider Component
**File:** `ge-app/include/ge-app/ui/slider.hpp`

**Usage:** Settings (Music, SFX)

**Features:**
- Configurable range
- Visual fill bar
- Joystick adjustment
- Volume conversion (0-255)

---

### Option Selector Component
**File:** `ge-app/include/ge-app/ui/option_selector.hpp`

**Usage:** Settings (Button Flip)

**Features:**
- Multiple options
- Arrow indicators
- Joystick navigation
- Current selection display

---

## Scene Management

### Scene Types
```cpp
enum class SceneType {
  Menu,      // Main menu
  Game,      // Gameplay
  Settings,  // Settings configuration
  Credits    // Credits display
};
```

### Scene Switching
All scene transitions are handled by `MainApp` in `ge-app/src/main.cpp`:

```cpp
void switch_to_menu();     // → Main Menu
void switch_to_game();     // → Game Scene
void switch_to_settings(); // → Settings Scene
void switch_to_credits();  // → Credits Scene
```

### Scene Lifecycle
1. **Construction**: Scene created with reference to MainApp
2. **Activation**: Pointer set as current_scene
3. **Update Loop**: tick() called every frame
4. **Rendering**: render() called every frame
5. **Input**: on_button_clicked() called on button press

---

## Input Flow

### Joystick Input
```
User moves joystick
        ↓
App::get_joystick_state()
        ↓
Scene::tick(float dt)
        ↓
UI Component processing
        ↓
Visual update / Action
```

### Button Input
```
User presses button
        ↓
App event detection
        ↓
Scene::on_button_clicked(Button btn)
        ↓
Action handling
        ↓
Scene transition (if applicable)
```

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────┐
│                    MainApp                       │
│  - Manages all scenes                           │
│  - Handles scene switching                      │
│  - Delegates input to current scene             │
└────────────┬────────────────────────────────────┘
             │
             ├──────────────┬──────────────┬───────────────┐
             ▼              ▼              ▼               ▼
      ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
      │  Menu    │  │  Game    │  │ Settings │  │ Credits  │
      │  Scene   │  │  Scene   │  │  Scene   │  │  Scene   │
      └────┬─────┘  └──────────┘  └────┬─────┘  └────┬─────┘
           │                            │             │
           │                            │             │
           ├────────────────────────────┴─────────────┘
           │
           ▼
    ┌─────────────────────────────────────┐
    │        UI Components                 │
    │  - Menu (reusable)                  │
    │  - Slider (reusable)                │
    │  - OptionSelector (reusable)        │
    └─────────────────────────────────────┘
```

---

## File Organization

```
ge-app/
├── include/ge-app/
│   ├── scenes/
│   │   ├── scene.hpp           (Base class)
│   │   ├── menu_scene.hpp      (Main menu)
│   │   ├── game_scene.hpp      (Gameplay)
│   │   ├── settings_scene.hpp  (Settings) ← NEW
│   │   └── credits_scene.hpp   (Credits)  ← NEW
│   └── ui/
│       ├── menu.hpp            (Menu component)
│       ├── slider.hpp          (Slider component) ← NEW
│       └── option_selector.hpp (Toggle component) ← NEW
└── src/
    └── main.cpp                (Scene management)
```

---

## Testing Flow

```
1. Start Application
        ↓
2. Main Menu appears
        ↓
3. Navigate to "Options"
        ↓
4. Enter Settings Scene
        ↓
5. Test each setting:
   - Music slider
   - SFX slider
   - Button flip toggle
        ↓
6. Return to Main Menu
        ↓
7. Navigate to "Credits"
        ↓
8. Enter Credits Scene
        ↓
9. View credits
        ↓
10. Return to Main Menu
```

See `TESTING_GUIDE.md` for detailed testing procedures.
