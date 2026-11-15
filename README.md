# Seam Carving (C++17, OpenCV, Qt 6)

Content-aware image shrinking. The GUI, the carver loop, the energy function and the tests are
written. **The three seam-finding algorithms are stubs for you to implement.**

## Layout

```
core/    OpenCV only, no Qt  ->  static library "seamcore"
gui/     Qt 6 Widgets        ->  seamcarve_app
tests/   Catch2 v3           ->  seamcore_tests
sample/  tower.png (synthetic test image)
docs/    AI_USAGE_NOTES.md   (for the AI declaration in your report)
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

## Quick start (Windows, Visual Studio 2026)

1. In the Visual Studio Installer, install **Desktop development with C++** and (Individual components)
   **vcpkg package manager**.
2. Open PowerShell in this folder and run `.\build.ps1`
   (if scripts are blocked: `powershell -ExecutionPolicy Bypass -File .\build.ps1`).
   The first run builds Qt and OpenCV from source through `vcpkg.json` (can take 30-90+ minutes; cached
   afterwards). `build.ps1` only builds; it does not run tests.
3. Run the app: `build\gui\Release\seamcarve_app.exe sample\tower.png` (the image argument is optional;
   you can also drag an image onto the window).
4. Run tests with `.\test.ps1` (see below).

`build.ps1` finds vcpkg by itself (`VCPKG_ROOT` if set, otherwise the copy bundled with Visual Studio),
then configures and builds. Run it again after every code change.

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

## Sharing a runnable app (no build needed on the other PC)

After `.\build.ps1`, run `.\deploy.ps1`. It creates `dist\seamcarve\` (exe, DLLs, Qt platform plugin, sample
image) and `dist\seamcarve-windows.zip`. Anyone who only wants to run the app can unzip it and start
`seamcarve_app.exe`. Only people who edit and test the code need the full build environment.
Smart App Control may still block the unsigned exe on a PC where it is on.

## Build manually (Windows, Visual Studio 2026 + vcpkg)

1. Install vcpkg (`git clone https://github.com/microsoft/vcpkg C:\vcpkg`, then `C:\vcpkg\bootstrap-vcpkg.bat`)
   and set `VCPKG_ROOT` to its folder (`setx VCPKG_ROOT "C:\vcpkg"`, then open a new terminal).
   Keep vcpkg up to date (`git pull` + bootstrap) so it recognises newer Visual Studio versions.
2. From PowerShell in this folder (the preset targets Visual Studio 2026; for another version change
   the `generator` line in `CMakePresets.json`, e.g. `Visual Studio 17 2022`):

```
cmake --preset windows
cmake --build --preset release
build\tests\Release\seamcore_tests.exe "[core]"
```

`vcpkg.json` pins a vcpkg baseline and builds only the Qt parts the app needs (`gui`, `widgets`, `png`,
`jpeg`), which keeps the first build shorter and avoids unused SQL drivers.

## Status

Built and run on Windows (Visual Studio 2026, vcpkg): the project compiles and the test results match
the expectations above. The test numbers (greedy trap 7 vs 103, brute-force oracle, `removedOrder`
bookkeeping) were also checked with standalone programs. The GUI itself has not been exercised beyond
launching it, so report anything odd (drag and drop, result window, cancel).
