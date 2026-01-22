# Testing Guide for Settings and Credits Scenes

This guide explains how to test the newly implemented Settings and Credits scenes.

## Prerequisites

Since this project requires SDL3 and is best built with Nix, you have two options:

### Option 1: Using Nix (Recommended)

```bash
# Enter the Nix development shell
nix develop

# Build for PC
cmake -S. -Bbuild
cmake --build build

# Run the application
./build/ge-app/Release/ge-app
```

### Option 2: Manual Setup

Install dependencies:
- CMake 3.20+
- Python 3.x
- SDL3 (from source or package manager)
- C++17 compatible compiler

Then build:
```bash
cmake -S. -Bbuild/pc
cmake --build build/pc
./build/pc/ge-app/Release/ge-app
```

## Testing the Settings Scene

### Navigation
1. Start the application
2. Use joystick Y-axis (or arrow keys if mapped) to select "Options"
3. Press Button 1 to enter Settings

### Music Volume Slider
1. In Settings, ensure "Music Volume" is selected (first item)
2. Move joystick X-axis left/right to adjust volume
3. Observe:
   - Slider bar fills/empties smoothly
   - Music volume changes in real-time
   - Value range is 0-100%

### SFX Volume Slider
1. Move joystick Y-axis down to select "SFX Volume"
2. Move joystick X-axis left/right to adjust volume
3. Observe:
   - Slider bar fills/empties smoothly
   - Value range is 0-100%
   - (Note: Actual SFX volume control requires additional audio system work)

### Button Flip Toggle
1. Move joystick Y-axis down to select "Button Layout"
2. Move joystick X-axis left/right to toggle
3. Observe:
   - "<" arrow appears when not at first option
   - ">" arrow appears when not at last option
   - Text changes between "Normal (A/B)" and "Flipped (B/A)"
   - (Note: Actual button remapping requires additional input system work)

### Back to Menu
1. Move joystick Y-axis down to select "Back to Menu"
2. Press Button 1
3. Observe:
   - Returns to main menu
   - Settings persist during session (volume remains changed)

## Testing the Credits Scene

### Navigation
1. From main menu, select "Credits"
2. Press Button 1 to enter Credits

### Display
Verify the following sections are displayed:
- Title: "Credits"
- Game Development: CTB Girls' Dorm
- Programming: Placeholder Developer
- Art & Graphics: Placeholder Artist
- Music: Placeholder Composer
- Special Thanks: Placeholder Team

### Back Button
1. Press Button 1 (back is the only option)
2. Observe: Returns to main menu

## Visual Verification

### Settings Scene
- [ ] Title "Settings" is centered near top
- [ ] All four items are visible and properly spaced
- [ ] Selected item has different color and border
- [ ] Sliders show visual fill bars
- [ ] Option selector shows current choice with arrows
- [ ] Background uses menu-bg texture

### Credits Scene
- [ ] Title "Credits" is centered near top
- [ ] All credit sections are visible
- [ ] Section headers are in black
- [ ] Contributor names are in gray
- [ ] Proper vertical spacing between sections
- [ ] Back button is at bottom
- [ ] Background uses menu-bg texture

## Edge Cases to Test

### Settings Scene
1. **Volume at 0%**: Slider should show empty bar
2. **Volume at 100%**: Slider should show full bar
3. **Rapid joystick movement**: Should not skip values or glitch
4. **Holding joystick left/right**: Volume should continuously adjust
5. **Joystick centered**: Should not trigger changes (dead zone)

### Credits Scene
1. **Quick navigation**: Should smoothly transition to/from menu
2. **Multiple back presses**: Should only trigger once per press

## Known Limitations

1. **SFX Volume**: The slider adjusts a value but doesn't yet control actual SFX volume (requires audio system enhancement)
2. **Button Flip**: The option toggles but doesn't yet remap buttons (requires input system enhancement)
3. **Settings Persistence**: Settings are not saved between sessions (requires save system)
4. **Placeholder Credits**: All credits are placeholders and should be replaced with actual contributors

## Automated Testing

The implementation will be tested in CI via GitHub Actions:
- Build for x86_64-linux (host)
- Build for ARM/STM32 (cross-compilation)
- Pre-commit checks

To run locally with Nix:
```bash
nix flake check
```

## Reporting Issues

When reporting issues, please include:
1. Platform (PC/SDL3 or STM32)
2. Steps to reproduce
3. Expected behavior
4. Actual behavior
5. Screenshots if applicable

## Next Steps for Full Implementation

To make these scenes fully functional:
1. Implement SFX volume control in audio system
2. Implement button remapping in input system
3. Add settings persistence (save to flash on STM32, config file on PC)
4. Replace placeholder credits with actual contributors
5. Consider adding more settings (brightness, difficulty, etc.)
6. Add sound effects for menu navigation
