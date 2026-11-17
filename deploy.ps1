# Packages the built app into SUBMIT\seamcarve (exe + DLLs + Qt plugins) and zips it, so people who
# only want to RUN the app do not need Qt/OpenCV/vcpkg installed.
# Run AFTER .\build.ps1 :   .\deploy.ps1
# If scripts are blocked: powershell -ExecutionPolicy Bypass -File .\deploy.ps1

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$buildDir = "build\gui\Release"
$exe = Join-Path $buildDir "seamcarve_app.exe"
if (-not (Test-Path $exe)) {
    Write-Host "Not found: $exe  - run .\build.ps1 first." -ForegroundColor Red
    exit 1
}

$SUBMIT = "SUBMIT\seamcarve"
if (Test-Path "SUBMIT") { Remove-Item -Recurse -Force "SUBMIT" }
New-Item -ItemType Directory -Force $SUBMIT | Out-Null

# 1. exe plus the DLLs vcpkg copied next to it (OpenCV, Qt core DLLs, ...)
Copy-Item (Join-Path $buildDir "*.exe") $SUBMIT
Copy-Item (Join-Path $buildDir "*.dll") $SUBMIT -ErrorAction SilentlyContinue
Copy-Item "image_sample" (Join-Path $SUBMIT "image_sample") -Recurse

# 2. Qt platform plugin. The trimmed vcpkg Qt has no windeployqt, so copy qwindows.dll directly:
#    first from the build folder (the build copies it there), otherwise from vcpkg's plugin folder.
$platDir = Join-Path $SUBMIT "platforms"
New-Item -ItemType Directory -Force $platDir | Out-Null
$plugin = Join-Path $buildDir "platforms\qwindows.dll"
if (-not (Test-Path $plugin)) {
    $plugin = "build\vcpkg_installed\x64-windows\Qt6\plugins\platforms\qwindows.dll"
}
if (Test-Path $plugin) {
    Copy-Item $plugin $platDir
} else {
    Write-Host "qwindows.dll not found - Qt plugin NOT deployed." -ForegroundColor Yellow
}

# 3. Sanity check: without the platform plugin the app dies at start-up with a cryptic message.
if (-not (Test-Path (Join-Path $SUBMIT "platforms\qwindows.dll"))) {
    Write-Host "WARNING: $SUBMIT\platforms\qwindows.dll is missing; the app will not start on another machine." -ForegroundColor Yellow
}

# 4. Zip it
$zip = "SUBMIT\seamcarve-windows.zip"
Compress-Archive -Path $SUBMIT -DestinationPath $zip -Force
Write-Host "`nFolder: $SUBMIT" -ForegroundColor Green
Write-Host "Zip:    $zip" -ForegroundColor Green
Write-Host "Test it: run $SUBMIT\seamcarve_app.exe. On the target PC, if Smart App Control is on it can still block the unsigned exe."
