# Superior

**Superior** هو أساس تطبيق Windows للعبة يعمل بأسلوب **capture-only**: يلتقط نافذة لعبة عبر Windows.Graphics.Capture ويعرضها في طبقة borderless فوقها. هذا هو **الطلب 1 / المرحلة 0.1.0** وليس محرك توليد إطارات أو رفع دقة عصبي مكتمل.

> لا يوجد ضمان عام لعبارة “anti-cheat safe”. لا يحقن Superior كوداً في اللعبة ولا يستخدم hooks أو تعريفات drivers أو صلاحيات مسؤول، لكن سياسات اللعبة وبرنامج الحماية قد تمنع الالتقاط أو الطبقة. راجع قواعد اللعبة قبل الاستخدام.

## What works in this stage

- C++17 / Direct3D 11 / x64 `Superior.exe`; Release uses static CRT `/MT`.
- Windows.Graphics.Capture free-threaded frame pool, GPU texture transfer, resize recreation, minimize pause/resume and target-close detection. **No CPU frame readback** in the capture/present path.
- Flip-model borderless overlay, waitable swap chain, vsync, click-through toggle and foreground/minimize visibility handling.
- HLSL compute scaling, with a vertex/pixel fullscreen fallback for GPUs without BGRA typed-UAV support. This is bilinear spatial scaling, **not** EASU, temporal SR or neural SR.
- Dark ImGui control panel, notification-area icon, customizable global hotkeys, JSON settings with validation, backup recovery and flushed replacement.
- Arabic/English text using Uniscribe bidi + HarfBuzz contextual shaping and actual glyph rasterization from an Arabic-capable Windows font. The primary labels and reading direction are mirrored; standard ImGui text-entry fields remain left-to-right for hotkey chords.
- Rotating write-through application log and a separate best-effort crash record.
- CPU-only GoogleTest coverage for settings, hotkey parsing and timing. CI does not create a D3D device or run GPU tests.

## Download and run

1. Open **GitHub → Actions → Build Superior → latest successful run**.
2. Download **Superior-windows-x64** and extract the entire ZIP. This is the recommended package.
3. Run `Superior.exe` on Windows 10 version 1903+ or Windows 11, x64, with a working DX11 GPU and Windows.Graphics.Capture support.
4. Set the game to **Borderless Windowed** (preferred) or Windowed. Select its window, then choose **Start capture / بدء الالتقاط**.
5. Return to the game. The overlay follows its window rectangle and hides when the game loses focus or minimizes.

The `Superior-exe` artifact contains only the executable. Stage 1 also needs the adjacent `shaders/*.cso` files, so use the ZIP unless you already have those files. Single-exe embedding is deliberately reserved for request 13.

No administrator elevation is requested. Windows may show its capture indicator. Do not disable that indicator or use capture on protected content you are not authorized to view.

## Hotkeys

| Action | Default |
|---|---|
| Show/hide control panel | Ctrl+Alt+U |
| Show/hide overlay | Ctrl+Alt+O |
| Toggle click-through | Ctrl+Alt+T |
| Start/stop capture | Ctrl+Alt+S |

Edit chords in the panel and choose **Apply hotkeys and save settings**. Conflicting combinations are reported. Use Ctrl, Alt or Win with a key, or F1–F24. Left-click the tray icon to reopen the panel; right-click for Exit.

Settings: `%LOCALAPPDATA%\Superior\settings.json`
Logs: `%LOCALAPPDATA%\Superior\logs\superior.log` and `crash.log`
Example settings: `config/settings.example.json` (copy to the settings path only if needed).

## Architecture and data flow

```text
Win32 control panel + ImGui / HarfBuzz / Uniscribe
                    |
Application ------- settings / hotkeys / logging / QPC timing
                    |
Game HWND -> Windows.Graphics.Capture -> D3D11 GPU texture
                                             |
                                  HLSL spatial scale
                                             |
                              waitable flip-model overlay
```

Implemented module boundaries: `core`, `capture`, `present`, `ui`, plus adapter identification in `hardware`. The remaining requested modules (`upscale`, `flow`, `interpolate`, `filters`, `oled`, `benchmark`) are later stages, not simulated implementations. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md), [docs/PLAN.md](docs/PLAN.md), and [docs/KNOWN_LIMITATIONS.md](docs/KNOWN_LIMITATIONS.md).

## Build and test — GitHub Actions only

The workflow `.github/workflows/build.yml` runs on `windows-latest`:

- CMake Configure: Visual Studio 17 2022, x64, Release.
- Build `Superior.exe` and compile HLSL with the Windows SDK `fxc`.
- Run `ctest` CPU-only tests; fail if no tests are discovered.
- Check executable imports for dynamic CRT dependencies.
- Upload `Superior.exe` and a portable ZIP containing shaders, example config, model-directory notice, documentation and dependency licenses.

Dependencies are fetched at pinned revisions with SHA-256 archive checks: Dear ImGui, nlohmann/json, GoogleTest and HarfBuzz. No local compiler is required to download and run a CI artifact. Project policy for this session is **not to run local builds**.

A green workflow means compilation, shader compilation, CPU tests and packaging passed. It does **not** mean capture, overlay visibility, Arabic rendering, latency, thermals or compatibility were tested on a real GPU.

## Manual acceptance test for request 1

- [ ] Launch without elevation; confirm the panel and tray icon appear.
- [ ] Switch Arabic/English; check joined Arabic letters, mixed Arabic/English and digits.
- [ ] Capture a moving windowed application/game; verify orientation, color and mouse input reach the game.
- [ ] Resize/move the window, switch monitors with different DPI, enter/leave borderless fullscreen.
- [ ] Minimize/restore; close the target; confirm capture stops/pauses without a stale overlay.
- [ ] Try exclusive fullscreen; confirm guidance rather than a claimed successful capture.
- [ ] Exercise all four hotkeys and a conflicting chord; restart and verify persisted settings.
- [ ] Check CPU/GPU load and temperatures with the game's own cap. Record actual observations; this stage makes no FPS-gain claim.

Report the GPU, Windows build, game display mode, artifact SHA-256 and relevant log lines with any failure. Do not upload logs containing private window titles or paths without reviewing them.
