# Seam Carving (C++17, OpenCV, Qt 6)

Content-aware image shrinking. The GUI, the carver loop, the energy function and the tests are
written. **The three seam-finding algorithms are stubs for you to implement.**

## Layout

```
core/         OpenCV only, no Qt  ->  static library "seamcore"
gui/          Qt 6 Widgets        ->  seamcarve_app
tests/        Catch2 v3           ->  seamcore_tests
image_sample/ Sample images for testing (tower.png, plant.jpg, deer.jpg, etc.)
SUBMIT/       Pre-packaged standalone release for marking (no build needed)
```

### What you implement

| File | Assignment item |
|---|---|
| `core/DpSeamFinder.cpp` | 1b dynamic programming |
| `core/GreedySeamFinder.cpp` | 3 greedy |
| `core/GraphCutSeamFinder.cpp` | 4 graph cut / max-flow (read the padding note in the `.hpp`) |

Each stub returns the middle column, so the app runs end to end before you start.
Interface: `Seam findVertical(const cv::Mat& img, const cv::Mat& energy)`; see `core/SeamFinder.hpp`
for the contract (valid seam, deterministic, leftmost tie-break, honest `Seam::cost`).

### Other pieces

- `core/Energy.*`: Sobel L1 energy (`CV_64F`, integer-valued) and an L2 "Gradient" mode.
- `core/SeamOps.*`: `isValidSeam`, `seamCost`, `removeVerticalSeam`, `transposeImage`, `overlaySeams`.
- `core/SeamCarver.*`: `carve()`; vertical seams first, then transpose and do horizontal ones.
  Shrink only. Fills `removedOrder` (which seam removed which original pixel) for visualisation.
- `gui/`: `MainWindow` (controls), `DropImageView` (drag and drop), `ResultDialog` (non-modal result
  window, one per run; resize it and the image is re-carved from the original to the new size, shrink
  only), `CarveWorker` (runs `carve()` on a thread), `MatQtConvert` (Mat/QImage helpers).

---

## Quick Start: Immediate Marking & Testing (No Build Required)

> [!IMPORTANT]
> **For Markers / Evaluators:** You do **not** need Visual Studio, vcpkg, or to compile Qt 6 and OpenCV from source (which normally takes 30–90+ minutes). A pre-built standalone package is provided in `SUBMIT/`.

1. **Open the pre-built folder or unzip**:
   - Folder: `SUBMIT\seamcarve\`
   - Archive: `SUBMIT\seamcarve-windows.zip`
2. **Launch the application**:
   - Double-click `SUBMIT\seamcarve\seamcarve_app.exe`.
3. **Test with sample images**:
   - Drag and drop images from `SUBMIT\seamcarve\image_sample\` (`tower.png`, `broadway_tower_small.png`, `plant.jpg`, `deer.jpg`, `tower_real.jpg`) directly into the application window or use "Open image...".
   - Select an algorithm (DP, Greedy, Graph Cut) and click **Run**.
   - Drag the edges of the resulting window to test interactive content-aware resizing.
4. **Run Unit Tests (Catch2)**:
   - Automated tests can be executed via `.\test.ps1` (e.g. `.\test.ps1 dp` or `.\test.ps1 -Full`).

> [!NOTE]
> If Windows Smart App Control or Defender prompts about an unrecognized app on a new PC, choose "Run anyway". All required runtime libraries (OpenCV 4, Qt 6, and `platforms\qwindows.dll`) are already bundled next to the executable.

---

## Quick Start: Building from Source (Visual Studio 2026)

1. Install **Visual Studio 2026** with the **Desktop development with C++** workload, and **Git**.
   Nothing else: no vcpkg install, no environment variables.
2. Open PowerShell in this folder and run `.\build.ps1`
   (if scripts are blocked: `powershell -ExecutionPolicy Bypass -File .\build.ps1`).
   The first run clones a private copy of vcpkg into `.vcpkg\` (a few hundred MB, git-ignored), builds Qt and
   OpenCV from source through `vcpkg.json` (can take 30-90+ minutes; cached afterwards). `build.ps1` only
   builds; it does not run tests.
3. Run the app: `build\gui\Release\seamcarve_app.exe image_sample\tower.png` (the image argument is optional;
   you can also drag an image onto the window).
4. Run tests with `.\test.ps1` (see below).
5. To re-package the standalone submission folder after building: run `.\deploy.ps1`.

`build.ps1` keeps vcpkg private to the project: it clones vcpkg into `.vcpkg\`, checks out the commit pinned
as `builtin-baseline` in `vcpkg.json`, bootstraps it, sets `VCPKG_ROOT` for that run only, then configures
and builds. Your system vcpkg is never touched, and everyone builds the same package versions. Run it again
after every code change. To use a vcpkg already on your machine (`VCPKG_ROOT`, or the one bundled with Visual
Studio) run `.\build.ps1 -SystemVcpkg`. To change library versions, replace the `builtin-baseline` hash in
`vcpkg.json` and rebuild.

### Windows Security / Smart App Control

Smart App Control blocks unsigned programs it does not recognise. If it is **On** it can stop vcpkg's
downloaded build tools and your own freshly built `.exe` files from running. Check
Windows Security > App & browser control > Smart App Control. It may need to be turned off on the
machine you build on (read the warning on that screen first: on some Windows versions it cannot be
turned back on without reinstalling Windows).

### Qt platform plugin error

`Could not find the Qt platform plugin "windows"` means `platforms\qwindows.dll` is not next to the exe.
The build copies it there after every build (the trimmed vcpkg Qt has no `windeployqt`). If you still see
the error, copy it by hand:

```
New-Item -ItemType Directory -Force build\gui\Release\platforms | Out-Null
Copy-Item build\vcpkg_installed\x64-windows\Qt6\plugins\platforms\qwindows.dll build\gui\Release\platforms\
```

---

## GUI & Qt Sizing Limitations

When evaluating and interacting with the Qt GUI application, keep in mind the following architectural sizing constraints and behavioral specifications:

1. **Strict Shrink-Only Constraint**:
   - The seam carving implementation operates purely on image reduction (carving / downscaling). Image enlargement / expansion (seam insertion) is intentionally unsupported.
   - Target dimensions in `MainWindow` (`widthSpin_` / `heightSpin_` and horizontal sliders) are constrained to the range `[1, original_width]` and `[1, original_height]`. Entering target dimensions equal to or exceeding the original input is disabled or rejected.

2. **Result Dialog Minimum Widget Dimensions**:
   - The dynamic result viewer (`ResultDialog`) features a central canvas (`imageLabel_`) with an enforced minimum size of **60 × 60 pixels** (`imageLabel_->setMinimumSize(60, 60)`). Qt layout policies prevent reducing the target image dimensions below $60 \times 60$ via mouse drag.
   - The result dialog enforces a minimum window boundary of **520 × 440 pixels** (`resize(std::max(w, 520), std::max(h, 440))`) to ensure that all control widgets (save/close buttons, seam overlay toggle checkbox, seam index slider, and status/progress labels) remain fully visible and usable.

3. **Window Dragging & Clamping**:
   - Resizing the `ResultDialog` by dragging window borders triggers dynamic on-the-fly seam recalculation.
   - If the window is expanded larger than the original image dimensions, the target dimensions are automatically clamped:
     `std::clamp(width, 1, original.cols)` and `std::clamp(height, 1, original.rows)`.
   - The image is never upsampled, stretched, or padded; carving only occurs within the bounds of the original source image.

4. **Screen Resolution Caps & Large Images**:
   - Upon opening `ResultDialog`, if an image is exceptionally large, the dialog window dimensions are automatically capped to the available screen geometry (`avail.width() - 80`, `avail.height() - 80`) so the window does not spill off-screen.
   - Large images incur higher per-seam computational costs across multiple passes. A 150 ms debounce timer (`debounce_->setInterval(150)`) is used during interactive resizing to ensure that re-carving only triggers once user border-dragging pauses, preventing GUI freezing.

---

## Tests

Run `.\build.ps1` first. `test.ps1` runs whatever was built last.

```
.\test.ps1                       # everything, short output (one line per failure)
.\test.ps1 -List                 # list groups and every test name
.\test.ps1 core                  # group: core | energy | ops | carver | contract | optimality
.\test.ps1 dp                    # one finder's optimality test: dp | greedy | graphcut
.\test.ps1 -Name "greedy trap"   # tests whose name contains this text
.\test.ps1 optimality -Full      # full Catch2 output (failing matrices, messages)
```

(If scripts are blocked: `powershell -ExecutionPolicy Bypass -File .\test.ps1 core`.)

Expected result with the shipped stubs (run on Windows): all `[core]` tests pass, and 4 fail:
DP optimality, graph-cut optimality, the "canyon" contract test and the "greedy trap". They need the real
algorithms. "DP and graph cut agree on a realistic image" passes only because both stubs return the same
middle column; do not trust it until the algorithms are in.

Graph-cut optimality needs the padding fix from `core/GraphCutSeamFinder.hpp`. In 73 of 200 random
6x5 matrices the true optimum uses the last column, which the unpadded construction cannot reach.

---

## Build manually (Windows, Visual Studio 2026 + vcpkg)

1. Run `.\build.ps1` once so `.vcpkg\` exists, then in the same PowerShell window set
   `$env:VCPKG_ROOT = "$PWD\.vcpkg"`. (Or use your own vcpkg clone, checked out at the `builtin-baseline`
   commit from `vcpkg.json`.)
2. From PowerShell in this folder (the preset targets Visual Studio 2026; for another version change
   the `generator` line in `CMakePresets.json`, e.g. `Visual Studio 17 2022`):

```
cmake --preset windows
cmake --build --preset release
build\tests\Release\seamcore_tests.exe "[core]"
```

`vcpkg.json` pins a vcpkg baseline and builds only the Qt parts the app needs (`gui`, `widgets`, `png`,
`jpeg`), which keeps the first build shorter and avoids unused SQL drivers.

---

## Status

Built and run on Windows (Visual Studio 2026, vcpkg): the project compiles and the test results match
the expectations above. The test numbers (greedy trap 7 vs 103, brute-force oracle, `removedOrder`
bookkeeping) were also checked with standalone programs. The GUI itself has not been exercised beyond
launching it, so report anything odd (drag and drop, result window, cancel).
