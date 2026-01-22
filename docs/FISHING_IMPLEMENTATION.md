# Fishing System Implementation

This document describes the fishing system implementation for the GE-Fangame project.

## Overview

The game now includes a fishing mode that allows players to cast a fishing line, wait for fish to bite, and catch them. The fishing system is integrated into the existing game modes and can be activated by switching to Fishing mode. All fishing feedback is displayed through on-screen dialog boxes with input focus.

## Architecture

### Components

1. **Fishing System** (`ge-app/include/ge-app/game/fishing.hpp`)
   - Manages fishing state machine (7 states: Idle, Casting, Fishing, FishBiting, Caught, BaitLost, Reeling)
   - Handles joystick flick detection for casting
   - Implements line drawing using Bresenham's algorithm
   - Renders bobber with floating and wiggle animations
   - Manages fish bite timing and player interaction
   - Integrates DialogBox for all user feedback

2. **Enhanced DialogBox** (`ge-app/include/ge-app/gfx/dialog_box.hpp`)
   - Added input focus system to prevent accidental actions
   - Methods: `show_message()`, `has_input_focus()`, `dismiss()`
   - Blocks other input when displaying messages
   - Supports fast-forwarding with Button A

3. **GameScene Integration** (`ge-app/include/ge-app/scenes/game_scene.hpp`)
   - Added fishing system to GameScene
   - Integrated fishing updates in Fishing mode
   - Renders fishing line, bobber, and dialog boxes
   - Handles input focus for fishing dialogs

### Fishing States

1. **Idle**: Not fishing, waiting for joystick flick
2. **Casting**: Animated line extending from boat to target position (0.25s with easing)
3. **Fishing**: Line in water, waiting for fish to bite
4. **FishBiting**: Fish is biting, player must react quickly
5. **Caught**: Fish caught, ready to be reeled in with Button A
6. **BaitLost**: Fish got away after eating all the bait, player must reel in empty line
7. **Reeling**: Bobber animating back to boat (0.8s)

## Key Features

### Joystick Flick Detection

- Tracks joystick magnitude change over time to detect "flicking" motion
- Velocity threshold: 3.0 units/second
- Minimum joystick magnitude: 0.4
- Cast distance based on joystick magnitude (up to 80 pixels)
- Cast direction based on joystick angle

### Visual Feedback

1. **Fishing Line**: Drawn using Bresenham's line algorithm in cyan color
   - Animates during casting (extends from boat to target)
   - Animates during reeling (retracts from target to boat)
2. **Bobber**: 3x3 pixel red square with animations:
   - Casting animation: Moves from boat to cast position (0.25s with ease-out cubic)
   - Floating effect: Sinusoidal up/down motion (±2 pixels)
   - Wiggle effect: Circular motion that increases when fish bites
   - Small wiggle (amplitude: 1px) during normal fishing
   - Large wiggle (amplitude: 5px) when fish is biting
   - Reeling animation: Returns to boat (0.8s)

### Fishing Mechanics

1. **Casting**:
   - Flick the joystick to cast in desired direction
   - Distance proportional to joystick magnitude
   - 0.25 second animated casting with smooth easing (line extends to target)

2. **Waiting**:
   - Fish can bite after 2 seconds minimum
   - 30% chance per second for fish to bite
   - Automatic timeout after 15 seconds (fish gets away)

3. **Catching**:
   - Player has 3.5 seconds to press Button A after fish bites
   - 0.5 second reaction time required before catching is possible
   - Visual feedback: Increased wiggle intensity
   - If time runs out: Transitions to BaitLost state with message "The fish ate all the bait and got away!"

4. **Bait Lost**:
   - State entered when fish escapes after eating bait (time expired in FishBiting)
   - Player must press Button A to reel in the empty line
   - Shows "Reeling in empty line..." when reeling starts
   - Provides visual feedback that bait was lost before resetting

5. **Early Retraction**:
   - Player can press Button A during Fishing or FishBiting states
   - Triggers reeling animation immediately
   - Shows "Reeling in early..." message
   - Results in "Nothing caught." after animation completes

6. **Reeling**:
   - 0.8 second animation showing bobber returning to boat
   - Handles three scenarios:
     - Fish caught: Logs fish name
     - Early retraction: Logs "Nothing caught."
     - Bait lost: Logs "Nothing caught."

7. **Rewards**:
   - Random fish or loot printed to stdout (only when caught in time)
   - Possible catches:
     - Tropical Fish
     - Golden Fish (RARE!)
     - Sea Bass
     - Tuna
     - Old Boot (loot)
     - Treasure Chest (RARE loot!)
     - Salmon
     - Pufferfish
     - Clownfish
     - Sardine

### Input Controls

- **Joystick**: Flick to cast fishing line
- **Button 2**: Switch between game modes (Steering/Fishing/Management)
- **Button 1**:
  - **When dialog has focus**: Dismiss dialog (or fast-forward if typing)
  - Catch fish when in Caught state (triggers reeling animation with fish)
  - Reel in empty line when in BaitLost state (triggers reeling animation, no catch)
  - Retract early during Fishing/FishBiting states (triggers reeling animation, no catch)

### User Feedback (Dialog System)

All fishing events display on-screen dialogs:
- **Fish Biting**: "Fish is biting! Press A to catch it!"
- **Bait Lost**: "The fish ate all the bait and got away!"
- **Early Retraction**: "Reeling in early..."
- **Empty Line**: "Reeling in empty line..."
- **No Catch**: "Nothing caught."
- **Success**: Fish name (e.g., "Golden Fish (RARE!)")
- **Timeout**: "The fish got away. Recast your line!"

Dialogs use input focus to prevent accidental actions while messages are displayed.

## Implementation Details

### Constants

```cpp
static constexpr float MIN_DELTA_TIME = 0.001f;          // Minimum time step
static constexpr float FLICK_THRESHOLD = 3.0f;           // Velocity for flick
static constexpr float MIN_JOYSTICK_MAG = 0.4f;          // Min joystick magnitude
static constexpr float CAST_DURATION = 0.25f;            // Cast animation time (faster)
static constexpr float REEL_DURATION = 0.8f;             // Reel animation time
static constexpr float BITE_CHANCE_PER_SECOND = 0.3f;    // Fish bite probability
static constexpr float MIN_FISHING_TIME = 2.0f;          // Min wait before bite
static constexpr float MAX_FISHING_TIME = 15.0f;         // Max wait (timeout)
static constexpr float BITE_WINDOW = 3.5f;               // Time to catch fish (3-4s)
static constexpr float CATCH_REACTION_TIME = 0.5f;       // Required reaction time
```

### Easing Functions

**Ease-Out Cubic** (for smooth casting animation):
```cpp
static float ease_out_cubic(float t) {
  float f = t - 1.0f;
  return f * f * f + 1.0f;
}
```

This creates a smooth deceleration effect where the bobber starts fast and slows down as it reaches the target position.

### Animation Details

**Casting Animation (with easing):**
```cpp
float progress = casting_timer / CAST_DURATION;
progress = ease_out_cubic(progress);  // Apply easing
bobber_x = cast_x * progress;  // 0.0 -> 1.0 (smooth)
bobber_y = cast_y * progress;
```

**Reeling Animation:**
```cpp
float progress = reeling_timer / REEL_DURATION;
float inv_progress = 1.0 - progress;  // 1.0 -> 0.0
bobber_x = cast_x * inv_progress;
bobber_y = cast_y * inv_progress;
```

### Line Drawing Algorithm

The fishing line uses Bresenham's line algorithm for efficient pixel-perfect line rendering:
- Draws from boat center to bobber position
- Handles coordinate transformation from relative to screen space
- Clips to screen boundaries automatically
- Updates dynamically during casting and reeling animations

### Bobber Animation

Bobber position is calculated using:
```cpp
wiggle_x = sin(wiggle_time * wiggle_freq) * wiggle_amplitude
wiggle_y = cos(wiggle_time * wiggle_freq * 0.7) * wiggle_amplitude
float_offset = sin(wiggle_time * 2.0) * 2.0
```

### Performance Optimizations

1. **No Dynamic Allocation**: All state stored in class members
2. **Efficient Line Drawing**: Uses integer arithmetic (Bresenham)
3. **Minimal State**: Only 7 states in state machine (Idle, Casting, Fishing, FishBiting, Caught, BaitLost, Reeling)
4. **Frame-based Updates**: All animations use delta time
5. **Smooth Animations**: Ease-out cubic easing for casting, linear interpolation for reeling
6. **Fast Casting**: 0.25s casting animation for responsive gameplay

## Files Modified

- `ge-app/include/ge-app/scenes/game_scene.hpp`: Added fishing integration
  - Added `#include "ge-app/game/fishing.hpp"`
  - Added `Fishing fishing;` member
  - Updated `tick()` to handle fishing mode
  - Updated `render()` to draw fishing elements and dialogs
  - Updated `on_button_clicked()` to handle fishing actions and dialog input focus

- `ge-app/include/ge-app/gfx/dialog_box.hpp`: Enhanced DialogBox API
  - Added input focus system
  - Added `show_message()`, `has_input_focus()`, `dismiss()` methods
  - Added `get_pending_message()` for rendering
  - Added `update()` and `is_message_complete()` for state management

## Files Added

- `ge-app/include/ge-app/game/fishing.hpp`: Complete fishing system implementation
  - Includes DialogBox integration
  - Handles all fishing states and transitions
  - Provides user feedback through on-screen dialogs

## Usage

1. **Switch to Fishing Mode**: Press Button 2 to cycle to Fishing mode
2. **Cast Line**: Flick the joystick in the desired direction
   - Watch the animated line extend to the target position with smooth easing (0.25s)
3. **Wait or Retract Early**:
   - Watch the bobber float and wiggle while waiting for a fish
   - Press Button A anytime to retract early (shows dialog: "Reeling in early...")
4. **React to Fish Bite**:
   - When fish bites, dialog appears: "Fish is biting! Press A to catch it!"
   - Press Button A to dismiss dialog, then press again to catch
   - Must catch within 3.5 seconds or fish escapes
5. **Bait Lost**:
   - If timeout, dialog shows: "The fish ate all the bait and got away!"
   - Press Button A to dismiss, then press again to reel in empty line
6. **Reel In**: Watch the bobber animate back to the boat (0.8s)
7. **Result Dialog**:
   - Success: Fish name displayed (e.g., "Golden Fish (RARE!)")
   - No catch: "Nothing caught."
   - Press Button A to dismiss and continue fishing

## Future Enhancements

Potential improvements for the fishing system:

1. **Inventory System**: Store caught fish in an inventory
2. **Fish Stats**: Track species, size, rarity, value
3. **Fishing Rod Upgrades**: Different rods with different cast distances
4. **Bait System**: Use different baits to attract specific fish
5. **Fish AI**: More sophisticated fish behavior patterns
6. **Sound Effects**: Audio feedback for casting, bites, catches
7. **Visual Effects**: Splash animations, ripples in water
8. **Fishing Spots**: Special locations with better fish
9. **Time/Weather Effects**: Fish behavior changes based on conditions
10. **Mini-game**: Button timing challenge when reeling in fish

## Testing

The implementation should be tested to verify:

- [ ] Mode switching works correctly
- [ ] Joystick flick detection is responsive
- [ ] Fishing line renders correctly
- [ ] Bobber floats and wiggles appropriately
- [ ] Fish bite timing is reasonable
- [ ] Button A catches fish when pressed in time
- [ ] Fish names are printed to stdout
- [ ] Timeout works when fish not caught
- [ ] State transitions are smooth
- [ ] Boat still drifts in fishing mode

Note: Due to build environment constraints, full testing requires the Nix development environment or manual setup of SDL3 and build tools. Testing on actual STM32 hardware is recommended for final verification.

## Known Limitations

1. **Random Number Generation**: Currently uses `rand()` which is not thread-safe. For production, consider using C++11 random facilities.
2. **No Inventory**: Caught fish are only printed to stdout, not stored.
3. **Simple Flick Detection**: Velocity-based detection may need tuning based on hardware.
4. **Fixed Bobber Texture**: Uses simple 3x3 red square instead of sprite.
5. **No Sound**: Fishing events are silent (only visual feedback).
