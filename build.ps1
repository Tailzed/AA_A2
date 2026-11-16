# One-step configure + build for Windows (Visual Studio 2026). Does NOT run tests: use .\test.ps1
# Run from PowerShell:   .\build.ps1
# If scripts are blocked: powershell -ExecutionPolicy Bypass -File .\build.ps1
#
# vcpkg: by default a private copy is cloned into .vcpkg\ (ignored by git) and checked out at the commit
# pinned in vcpkg.json ("builtin-baseline"). Nothing on your system is changed and everyone builds the
# same package versions. Use -SystemVcpkg to use VCPKG_ROOT or Visual Studio's bundled vcpkg instead.
param([switch]$SystemVcpkg)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Fail($message) {
    Write-Host $message -ForegroundColor Red
    exit 1
}

# --- private vcpkg in .vcpkg\ , pinned to the baseline commit from vcpkg.json --------------------------
function Ensure-LocalVcpkg {
    # Native commands: check $LASTEXITCODE ourselves instead of letting stderr output abort the script.
    $ErrorActionPreference = "Continue"

    if (-not (Get-Command git -ErrorAction SilentlyContinue)) { Fail "git is not installed or not on PATH." }

    $baseline = (Get-Content "vcpkg.json" -Raw | ConvertFrom-Json).'builtin-baseline'
    if (-not $baseline) { Fail "vcpkg.json has no builtin-baseline." }
    $dir = Join-Path $PSScriptRoot ".vcpkg"

    if (-not (Test-Path (Join-Path $dir ".git"))) {
        Write-Host "Cloning vcpkg into .vcpkg (one time, a few hundred MB)..." -ForegroundColor Cyan
        git clone https://github.com/microsoft/vcpkg $dir
        if ($LASTEXITCODE -ne 0) { Fail "git clone of vcpkg failed." }
    }

    git -C $dir cat-file -e "$baseline^{commit}" 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Fetching the pinned vcpkg commit..." -ForegroundColor Cyan
        git -C $dir fetch origin
        git -C $dir cat-file -e "$baseline^{commit}" 2>$null
        if ($LASTEXITCODE -ne 0) {
            Fail "Commit $baseline was not found in .vcpkg. Delete the .vcpkg folder and run .\build.ps1 again."
        }
    }

    git -C $dir checkout --quiet $baseline
    if ($LASTEXITCODE -ne 0) { Fail "git checkout $baseline failed in .vcpkg." }

    # (Re)bootstrap when vcpkg.exe is missing or was built for a different commit.
    $marker = Join-Path $dir ".bootstrapped"
    $done = (Test-Path (Join-Path $dir "vcpkg.exe")) -and (Test-Path $marker) -and
            ((Get-Content $marker -Raw).Trim() -eq $baseline)
    if (-not $done) {
        Write-Host "Bootstrapping vcpkg..." -ForegroundColor Cyan
        # Out-Host: show the bootstrap output without it becoming part of this function's return value.
        & (Join-Path $dir "bootstrap-vcpkg.bat") -disableMetrics | Out-Host
        if ($LASTEXITCODE -ne 0) { Fail "bootstrap-vcpkg failed." }
        Set-Content -Path $marker -Value $baseline
    }
    $script:LocalVcpkgRoot = $dir   # not "return": any stray output would end up in the return value
}

# --- optional: a vcpkg that is already on the machine (-SystemVcpkg) ------------------------------------
function Find-SystemVcpkg {
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

if ($SystemVcpkg) {
    $root = Find-SystemVcpkg
    if (-not $root) { Fail "No system vcpkg found (set VCPKG_ROOT or install Visual Studio's vcpkg component). Run without -SystemVcpkg to use the private copy." }
} else {
    Ensure-LocalVcpkg | Out-Host
    $root = $script:LocalVcpkgRoot
    if (-not ($root -is [string]) -or -not (Test-Path (Join-Path $root "scripts\buildsystems\vcpkg.cmake"))) {
        Fail "The private vcpkg in .vcpkg is not usable. Delete the .vcpkg folder and run .\build.ps1 again."
    }
}
$env:VCPKG_ROOT = $root   # only for this script run; nothing is saved to your system
Write-Host "Using vcpkg at $root" -ForegroundColor Cyan

# A build folder configured with a different vcpkg keeps the old toolchain path in its CMake cache.
$cache = "build\CMakeCache.txt"
if (Test-Path $cache) {
    $want = $root -replace '\\', '/'
    if (-not (Select-String -Path $cache -SimpleMatch -Pattern $want -Quiet)) {
        Write-Host "The build folder was configured with a different vcpkg; resetting its CMake cache." -ForegroundColor Yellow
        Remove-Item $cache -Force
        Remove-Item "build\CMakeFiles" -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host "`n== Configure (first run builds Qt/OpenCV and can take a long time) ==" -ForegroundColor Cyan
cmake --preset windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`n== Build ==" -ForegroundColor Cyan
cmake --build --preset release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nBuilt. Next:" -ForegroundColor Green
Write-Host "  App:    build\gui\Release\seamcarve_app.exe"
Write-Host "  Tests:  .\test.ps1        (see .\test.ps1 -List for options)"
