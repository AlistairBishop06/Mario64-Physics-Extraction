param(
    [string]$InstallDir = "third_party/libsm64"
)

$ErrorActionPreference = "Stop"

Write-Host "Setting up libsm64 source in $InstallDir"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git is required"
}

if (-not (Test-Path $InstallDir)) {
    git clone https://github.com/libsm64/libsm64.git $InstallDir
} else {
    Push-Location $InstallDir
    git pull
    Pop-Location
}

$make = Get-Command make -ErrorAction SilentlyContinue
if (-not $make) {
    $make = Get-Command mingw32-make -ErrorAction SilentlyContinue
}

if (-not $make) {
    Write-Host ""
    Write-Host "libsm64 source is present, but no MSYS2/MinGW make tool was found."
    Write-Host "Install MSYS2 MinGW64, then run this script from an MSYS2 MinGW64 shell or ensure mingw32-make is on PATH."
    Write-Host "Expected runtime DLL after build: $InstallDir/dist/sm64.dll"
    exit 1
}

Copy-Item "roms\Super Mario 64 (USA).z64" "$InstallDir\baserom.us.z64" -ErrorAction SilentlyContinue

Push-Location $InstallDir
& $make.Source lib
Pop-Location

if (-not (Test-Path "$InstallDir\dist\sm64.dll")) {
    throw "libsm64 build finished but dist/sm64.dll was not found"
}

Write-Host "Built $InstallDir/dist/sm64.dll"
Write-Host "Restart sm64_physics_sandbox.exe; it will use the libsm64 backend automatically."
