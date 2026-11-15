# Runs the tests you pick. Run .\build.ps1 first (and again after every code change).
#
#   .\test.ps1                    short summary of everything (one line per failure)
#   .\test.ps1 -List              list the groups and every test name
#   .\test.ps1 core               one group: core | energy | ops | carver | contract | optimality
#   .\test.ps1 dp                 one finder's optimality test: dp | greedy | graphcut
#   .\test.ps1 -Name "greedy trap"   tests whose name contains this text
#   .\test.ps1 optimality -Full   full Catch2 output (all failing matrices and messages)
#
# If scripts are blocked: powershell -ExecutionPolicy Bypass -File .\test.ps1 core
param(
    [Parameter(Position = 0)][string]$Group,
    [string]$Name,
    [switch]$List,
    [switch]$Full
)

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$exe = "build\tests\Release\seamcore_tests.exe"
if (-not (Test-Path $exe)) {
    Write-Host "Not found: $exe  - run .\build.ps1 first." -ForegroundColor Red
    exit 1
}

if ($List) {
    Write-Host "Groups (tags):" -ForegroundColor Cyan
    & $exe --list-tags
    Write-Host "`nAll tests:" -ForegroundColor Cyan
    & $exe --list-tests
    exit 0
}

# Catch2 filter: a tag like "[core]", or a name wildcard like "*greedy trap*"
if ($Name)       { $filter = "*$Name*" }
elseif ($Group)  { $filter = "[$($Group.Trim('[]'))]" }
else             { $filter = "" }

$catchArgs = @()
if ($filter) { $catchArgs += $filter }
if (-not $Full) { $catchArgs += @("-r", "compact") }   # compact: one line per failure, summary at the end

Write-Host "Running: seamcore_tests $($catchArgs -join ' ')" -ForegroundColor Cyan
& $exe @catchArgs
$code = $LASTEXITCODE

if ($code -eq 0) { Write-Host "`nAll selected tests passed." -ForegroundColor Green }
else { Write-Host "`nSome tests failed. Use -Full for details, or pick one with -Name." -ForegroundColor Yellow }
exit $code
