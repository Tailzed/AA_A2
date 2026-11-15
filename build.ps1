# One-step configure + build for Windows (Visual Studio 2026 + vcpkg). Does NOT run tests: use .\test.ps1
# Run from PowerShell:   .\build.ps1
# If scripts are blocked: powershell -ExecutionPolicy Bypass -File .\build.ps1
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

# Find vcpkg: use VCPKG_ROOT if set and valid, otherwise the copy bundled with Visual Studio.
function Find-Vcpkg {
    if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake")) {
        return $env:VCPKG_ROOT
    }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -prerelease -products * -property installationPath
        if ($vs) {
            $bundled = Join-Path $vs "VC\vcpkg"
            if (Test-Path "$bundled\scripts\buildsystems\vcpkg.cmake") { return $bundled }
        }
    }
    return $null
}

$root = Find-Vcpkg
if (-not $root) {
    Write-Host "Could not find vcpkg." -ForegroundColor Red
    Write-Host "Either tick 'vcpkg package manager' in the Visual Studio Installer (Individual components),"
    Write-Host "or run:  git clone https://github.com/microsoft/vcpkg C:\vcpkg ; C:\vcpkg\bootstrap-vcpkg.bat"
    Write-Host "and then:  `$env:VCPKG_ROOT = 'C:\vcpkg'"
    exit 1
}
$env:VCPKG_ROOT = $root
Write-Host "Using vcpkg at $root" -ForegroundColor Cyan

Write-Host "`n== Configure (first run builds Qt/OpenCV and can take a long time) ==" -ForegroundColor Cyan
cmake --preset windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n== Build ==" -ForegroundColor Cyan
cmake --build --preset release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nBuilt. Next:" -ForegroundColor Green
Write-Host "  App:    build\gui\Release\seamcarve_app.exe"
Write-Host "  Tests:  .\test.ps1        (see .\test.ps1 -List for options)"
