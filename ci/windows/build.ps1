#!/usr/bin/env pwsh
# Configure and build a CMake preset on native Windows (MSVC + Ninja).
#
# Default tree: build/windows/CY<year>/<config>
#
# Usage: ci/windows/build.ps1 2026 debug
param(
    [Parameter(Mandatory = $true)][ValidateSet("2026")][string]$Year,
    [Parameter(Mandatory = $true)][ValidateSet("debug", "release", "sanitize")][string]$Config
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $root

if (-not $env:DAILYBOY_BUILD_ROOT) { $env:DAILYBOY_BUILD_ROOT = "windows/" }
$env:DAILYBOY_VFX_PLATFORM = $Year
if (-not $env:CMAKE_POLICY_VERSION_MINIMUM) { $env:CMAKE_POLICY_VERSION_MINIMUM = "3.5" }

$preset = "cy$Year-$Config"
$binaryDir = "build/$($env:DAILYBOY_BUILD_ROOT)CY$Year/$Config"

if (-not $env:CMAKE_C_COMPILER_LAUNCHER) {
    $ccache = Get-Command ccache -ErrorAction SilentlyContinue
    if ($ccache) {
        $env:CMAKE_C_COMPILER_LAUNCHER = "ccache"
        $env:CMAKE_CXX_COMPILER_LAUNCHER = "ccache"
    }
}

$jobs = [Environment]::ProcessorCount
if ($jobs -lt 1) { $jobs = 4 }

$extra = @("-DDAILYBOY_ENABLE_COVERAGE=OFF")
$pyCandidates = @(
    "$env:LocalAppData\Programs\Python\Python313\python.exe",
    "C:\Python313\python.exe"
)
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue
if ($pythonCmd) { $pyCandidates += $pythonCmd.Source }
foreach ($candidate in $pyCandidates) {
    if ($candidate -and (Test-Path -LiteralPath $candidate)) {
        $ver = & $candidate -c "import sys; print('%d.%d' % sys.version_info[:2])"
        if ($ver -eq "3.13") {
            $extra += "-DPython3_EXECUTABLE=$candidate"
            break
        }
    }
}

if (-not $env:MSYS2_BASH) {
    foreach ($bash in @("C:\msys64\usr\bin\bash.exe", "C:\tools\msys64\usr\bin\bash.exe")) {
        if (Test-Path -LiteralPath $bash) {
            $env:MSYS2_BASH = $bash
            break
        }
    }
}

Write-Host "cmake --preset $preset $($extra -join ' ')"
& cmake --preset $preset @extra
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "cmake --build --preset $preset -j $jobs"
& cmake --build --preset $preset -j $jobs
exit $LASTEXITCODE
