# Delivery plan

## Current scope: request 1

1. Establish Superior naming, CMake/CI, core settings/time/logging.
2. Implement capture-only WGC and a waitable borderless presenter.
3. Add control panel, tray, hotkeys and shaped Arabic/English text.
4. Build and run CPU tests on GitHub Actions; publish EXE and portable ZIP.
5. Report implemented / untested / known issues, then stop for user feedback.

## Data flow

`Game window → WGC GPU frame → owned GPU texture → spatial scale → overlay`

Control messages and persisted settings stay outside the frame hot path. No CPU frame readback is used.

## Risks to validate on hardware

- WGC can be unavailable or blocked by policy; exclusive fullscreen and protected surfaces are not supported.
- A capture indicator may remain visible by design.
- Hybrid laptops may render/capture on different adapters; automatic game-adapter selection is request 2.
- Overlay and vsync behavior can add latency. Capture FPS is not generated FPS or game FPS.
- GPU copies, font atlas size and UI refresh cost need measurement; no power or performance gain is asserted.
- Device removal currently requires restarting the application.

## Later requests (not implemented in stage 1)

2 hardware/budget/governor; 3 comfort governor/work skipping; 4 spatial/temporal/neural SR;
5 flow/interpolation; 6 filters/OLED emulation; 7 advanced experiments; 8 benchmarks;
9 polish/installer/release; 10 SR v2; 11 interpolation v2; 12 measured verification;
13 embedded-shader single-exe packaging.

No later-stage feature is represented by fake results or a simulated implementation.
