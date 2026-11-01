# Superior architecture — stage 1

## Ownership and threading

The UI thread owns the D3D11 immediate context, presenter, control panel and settings. The WGC free-threaded callback only signals an event. Frame acquisition and GPU copies run on the application thread. The D3D11 multithread protection flag is enabled for WGC interoperability. COM and WinRT objects use smart pointers; HWND/HANDLE/menu lifetimes are owned by their wrappers.

`Application` installs its message handler on the control window after `Interface` constructs it, then forwards unhandled messages to ImGui/Win32. This is the dispatch path for hotkeys, tray messages and session shutdown.

## Capture and presentation

WGC uses a three-buffer B8G8R8A8 frame pool. Only the latest available frame is retained; older frames are closed. A same-context GPU `CopyResource` transfers it into an application-owned texture before returning the WGC frame to the pool. This avoids CPU readback and avoids returning a texture while a GPU copy may still reference it. It costs one GPU copy and VRAM allocation; later zero-copy/lifetime-fence work must be benchmarked rather than assumed faster.

Content-size changes recreate the frame pool and owned texture. Minimization closes the capture session; restoration starts a new session. Closing the target stops capture. Foreground changes hide the overlay. DPI changes are handled through a PerMonitorV2 manifest and physical window rectangles.

The overlay uses a three-buffer flip-discard waitable swap chain with maximum frame latency one and vsync. Compute writes directly to the back buffer if BGRA typed UAV stores are supported; otherwise the same spatial sampler runs through a fullscreen triangle. There is no frame interpolation, game-motion-vector access or injection.

## Text

The app loads Tahoma, then Arial, from the Windows fonts directory and rejects fonts without Arabic coverage. Uniscribe determines bidi runs and visual ordering; HarfBuzz shapes each run using the font's actual OpenType glyph IDs. Glyph bitmaps are packed into the ImGui atlas, preserving mark positioning and ligatures. This is not a replacement of Arabic letters with disconnected presentation-form characters. Primary labels and custom controls use RTL layout; numeric/hotkey input remains LTR.

## Failure behavior

Windows/D3D operations report failures to the control panel and log. Capture errors stop the session and hide the overlay. Device loss is reported and the app exits rather than pretending recovery succeeded. Settings use a size-limited parser, backup, flushed temporary file and replacement. Logs rotate at 1 MiB with three files. The crash record is best-effort: a process-wide crash or power failure cannot guarantee a final write.

## Verification boundary

CPU tests exercise logic only. CI has no interactive game, no target laptop and no GPU acceptance test. Font appearance, WGC behavior, hybrid-adapter selection, fullscreen transitions and performance require the manual checklist in README.
