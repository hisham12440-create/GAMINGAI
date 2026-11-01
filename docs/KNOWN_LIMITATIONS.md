# Known limitations — Superior 0.1.0 / request 1

- This is a capture/display foundation, not a finished production game enhancer.
- No generated frames, optical flow, neural SR, temporal SR, filters, OLED mode or benchmark claims yet.
- Spatial scaling is bilinear. It cannot reconstruct missing game detail or increase the game's rendering FPS.
- Windows 10 1903+ / Windows 11 x64 and WGC-capable DX11 hardware are required. Older Windows 10 releases are not supported by this free-threaded WGC path.
- Use borderless/windowed mode. Exclusive-fullscreen detection is an OS notification-state hint, not a universal detector. Protected content may appear black or fail.
- The system capture border is intentionally retained. Capture-only does not guarantee acceptance by every anti-cheat or game policy.
- The overlay is opaque within the target rectangle; it is not a transparent per-pixel HUD overlay. It hides when the target loses foreground focus.
- Device creation uses the default DXGI adapter. Matching the game's rendering adapter and hybrid-GPU optimization are request 2.
- Device loss requires an application restart. No claim of transparent driver-reset recovery is made.
- Stage 1 requires adjacent compiled shader files. The standalone EXE artifact is not self-contained; use the ZIP. Embedding is request 13.
- Arabic shaping/bidi is implemented, but full IME/editing behavior and every ImGui decoration are not fully localized. Hotkey fields are LTR.
- Logging is write-through and crash records are best-effort; no crash dump or remote upload is performed.
- CI verifies build, HLSL compilation, CPU logic, static CRT imports and packaging only. Real-GPU behavior and the manual checklist remain unverified until run on the user's laptops.
