# 3D Maze Generator (C++ / Qt6 / OpenGL)
## Features
- Cross-platform Qt 6 GUI with multi-touch (pinch/pan/rotation)
- 3D renderer with Orbit and First-Person cameras
- Maze generation: Recursive Backtracking, Prim’s; size/seed controls and live preview with progress/FPS
- BFS solver with red path visualization
- Precise first-person collision (wall-flag–based) with adjustable radius and sliding
- Themes: Flat (solid colors) and Textured (procedural wall/floor), ambient+diffuse+specular lighting, gamma correction
- Persistence: Save/Load JSON (auto-updates UI dims/algorithm/seed; error dialogs), Export PNG snapshot
- Mobile-ready: default 15×15 and MSAA=2 on Android/iOS; packaging guides in README

Cross-platform GUI application written in C++ using Qt6 for UI and multi-touch, and OpenGL for 3D rendering. Supports mouse, keyboard, and touch gestures. This project includes:
- Qt6 Widgets + QOpenGLWidget setup
- Touch gesture handling (pinch, pan)
- Basic app skeleton with menus, toolbar, and status bar

## Build Instructions

### Prerequisites
- CMake \>= 3.16
- C++17 compiler (MSVC, Clang, or GCC)
- Qt6 (Widgets, OpenGLWidgets modules)

### Windows (MSVC + CMake + Qt6)
1. Install Qt6 and MSVC (Visual Studio). Ensure `Qt6_DIR` is set or use CMake presets/toolchain.
2. Configure and build:
   ```bash
   cd maze3d/build
   cmake -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvcXXXX_64" ..
   cmake --build .
   ```

### macOS (Clang + Homebrew Qt6)
1. Install Qt6 via Homebrew: `brew install qt6`
2. Configure and build:
   ```bash
   cd maze3d/build
   cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt" ..
   cmake --build .
   ```

### Linux (GCC/Clang + Qt6)
1. Install Qt6 dev packages (e.g., `qt6-base-dev`)
2. Configure and build:
   ```bash
   cd maze3d/build
   cmake ..
   cmake --build .
   ```

### Run
The built executable is `Maze3DGenerator` in `build/` or your CMake build directory.

## Notes
- On platforms without Qt6, adjust `find_package(Qt6 ...)` or point CMake to your Qt6 install via `CMAKE_PREFIX_PATH`.
- OpenGL minimum is 3.3 core profile, adjustable in `main.cpp`.

## Controls

- Orbit Camera: Left-drag to rotate, mouse wheel or pinch to zoom, two-finger pan to adjust angle; rotation gesture adjusts yaw.
- First-Person: Press C to toggle, W/A/S/D to move, drag or two-finger pan to turn, wheel/pinch to move forward/back.
- Sensitivity: Defaults are set; can be adjusted programmatically via `MazeGLWidget::setSensitivity(...)`.

## Persistence
- Save maze to JSON (File > Save JSON) including dimensions, seed, algorithm, and walls.
- Load maze from JSON (File > Load JSON). Dimensions must match current grid; otherwise regenerate with the JSON config first.
- Export a PNG snapshot (File > Export PNG).

## Build Notes
- Ensure Qt6 modules Widgets and OpenGLWidgets are installed.
- If your distribution splits Qt6 packages, install qt6-base, qt6-base-dev, qt6-tools (for qmake), and ensure CMake can find Qt via CMAKE_PREFIX_PATH.
- If OpenGL 3.3 is unavailable, lower the version in main.cpp's QSurfaceFormat.

## Known Limitations
- JSON load requires matching dimensions if loading directly into current grid; the GUI workflow auto-updates dimensions before applying walls.
- Touch gesture support depends on platform touch driver and Qt configuration.

## Roadmap (optional improvements)
- Precise wall collision and navigation mesh for first-person mode.
- Textured materials and dynamic lighting/shadows.
- Additional generation algorithms (Kruskal, Eller) and seed management UI.
- Non-const grid accessors to simplify JSON load without const_cast.

## Rendering Themes
- Flat: solid-color materials with ambient+diffuse+specular and gamma correction.
- Textured: procedural wall/floor textures with tint, ambient+diffuse+specular lighting, and gamma correction.
- Toggle via Controls dock (Theme: Flat/Textured).

## Mobile Packaging (Android / iOS)

### Android (Qt 6 + Gradle)
Prerequisites:
- Android Studio installed (SDK + NDK) and Java JDK
- Qt 6 for Android (arm64‑v8a kit at minimum)
- Environment variables set: ANDROID_SDK_ROOT and ANDROID_NDK_ROOT

Step‑by‑step:
1) Install Qt 6 for Android and ensure the Android kits appear in Qt Creator (e.g., "Android Qt 6.x Clang arm64‑v8a").
2) Open this project in Qt Creator → Projects → Select the Android kit.
3) Configure signing (optional for debug): in Projects → Build Settings, set Keystore and alias for Release builds.
4) Build & Deploy:
   - Click Build → Run (Qt Creator will invoke Gradle to produce an APK/AAB).
   - On first run, Gradle may download dependencies; allow time.
5) Verify on device/emulator:
   - Multi‑touch gestures (pinch/pan/rotation) should work.
   - Default maze size is reduced to 15×15; MSAA samples are set to 2.

Troubleshooting:
- If NDK mismatch errors occur, align the NDK version in Qt Creator with the Android kit’s required NDK.
- If signing fails, use a debug build or configure keystore properly.

### iOS (Qt 6 + Xcode)
Prerequisites:
- macOS with Xcode installed
- Qt 6 for iOS kits available in Qt Creator
- Valid Apple developer account/provisioning (for device deployment)

Step‑by‑step:
1) Open the project in Qt Creator → Projects → Select "iOS Qt 6.x" kit.
2) In Build Settings, choose Simulator or a connected Device target.
3) For device builds, ensure signing/provisioning profiles are set in Xcode (Qt Creator will hand off to Xcode).
4) Build & Run:
   - Click Run to launch on simulator/device.
5) Verify:
   - Pinch/pan/rotation gestures work.
   - Default maze size is 15×15; MSAA samples set to 2.

Troubleshooting:
- If code signing errors occur, open the generated Xcode project and set Signing & Capabilities appropriately.
- Use Release builds for performance on real devices.

### Notes
- This project uses CMake; mobile packaging is managed by Qt Creator kits that integrate with Gradle (Android) and Xcode (iOS).
- Shaders target OpenGL ES 3.x via Qt; on devices reporting lower versions, reduce features (e.g., specular) if needed.

## First-Person Collision
- Precise wall-flag–based collision using cell edges and an adjustable player radius.
- Axis-separated movement enables smooth sliding along walls and corners.
- Adjust the radius via MazeGLWidget::setCollisionRadius(r) to tune the feel.