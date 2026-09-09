#!/usr/bin/env pwsh
# Run unit / sanitize / perf / load tests on native Windows.
#
# Usage: ci/windows/test.ps1 <2025|2026> tests
param(
    [Parameter(Mandatory = $true)][ValidateSet("2025", "2026")][string]$Year,
    [Parameter(Mandatory = $true)][ValidateSet("tests", "sanitize", "perf", "load")][string]$Kind
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Set-Location $root

$env:DAILYBOY_BUILD_ROOT = if ($env:DAILYBOY_BUILD_ROOT) { $env:DAILYBOY_BUILD_ROOT } else { "windows/" }
$env:DAILYBOY_VFX_PLATFORM = $Year

switch ($Kind) {
    "tests" {
        $binaryDir = "build/$($env:DAILYBOY_BUILD_ROOT)CY$Year/debug"
        $ctestPreset = "cy$Year-tests"
    }
    "sanitize" {
        $binaryDir = "build/$($env:DAILYBOY_BUILD_ROOT)CY$Year/sanitize"
        $ctestPreset = "cy$Year-sanitize"
    }
    { $_ -in @("perf", "load") } {
        $binaryDir = "build/$($env:DAILYBOY_BUILD_ROOT)CY$Year/release"
        $ctestPreset = $Kind
    }
}

$depsBins = @()
if (Test-Path "$binaryDir/_deps") {
    $depsBins = Get-ChildItem -Path "$binaryDir/_deps" -Directory -Recurse -Filter "bin" -ErrorAction SilentlyContinue |
        ForEach-Object { $_.FullName }
}
$depsLibs = @()
if (Test-Path "$binaryDir/_deps") {
    $depsLibs = Get-ChildItem -Path "$binaryDir/_deps" -Directory -Recurse -Filter "lib" -ErrorAction SilentlyContinue |
        ForEach-Object { $_.FullName }
}

$pathParts = @()
$pathParts += $depsBins
$pathParts += $depsLibs
$pathParts += (Join-Path $root "$binaryDir\bin")
$pathParts += (Join-Path $root "$binaryDir\dailyboy")
$pathParts += (Join-Path $root "$binaryDir\api\cpp")
$env:Path = (($pathParts | Where-Object { $_ -and (Test-Path $_) }) -join ";") + ";" + $env:Path

& ctest --preset $ctestPreset
exit $LASTEXITCODE
