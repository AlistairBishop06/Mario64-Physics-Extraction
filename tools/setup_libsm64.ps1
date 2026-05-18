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
    $clang = Get-Command x86_64-w64-mingw32-clang -ErrorAction SilentlyContinue
    if (-not $clang) {
        $ghcupClang = "C:\ghcup\ghc\9.6.7\mingw\bin\x86_64-w64-mingw32-clang.exe"
        if (Test-Path $ghcupClang) {
            $clang = Get-Item $ghcupClang
        }
    }

    if (-not $clang) {
        Write-Host ""
        Write-Host "libsm64 source is present, but no MSYS2/MinGW make tool or MinGW clang was found."
        Write-Host "Install MSYS2 MinGW64, then run this script from an MSYS2 MinGW64 shell or ensure mingw32-make is on PATH."
        Write-Host "Expected runtime DLL after build: $InstallDir/dist/sm64.dll"
        exit 1
    }
}

$rom = Get-ChildItem "roms" -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Extension -in @(".z64", ".n64", ".v64") } |
    Select-Object -First 1
if ($rom) {
    Copy-Item $rom.FullName "$InstallDir\baserom.us.z64" -Force
}

Push-Location $InstallDir
if ($make) {
    & $make.Source lib
} else {
    if (Get-Command python -ErrorAction SilentlyContinue) {
        python import-mario-geo.py
    } elseif (Get-Command py -ErrorAction SilentlyContinue) {
        py import-mario-geo.py
    } else {
        throw "python is required to generate libsm64 Mario geometry sources"
    }

    New-Item -ItemType Directory -Force dist, dist\include | Out-Null
    $sourceDirs = @(
        "src",
        "src/decomp",
        "src/decomp/engine",
        "src/decomp/game",
        "src/decomp/pc",
        "src/decomp/pc/audio",
        "src/decomp/tools",
        "src/decomp/audio",
        "src/decomp/mario"
    )
    $files = foreach ($dir in $sourceDirs) {
        if (Test-Path $dir) {
            Get-ChildItem $dir -Filter *.c | ForEach-Object {
                $_.FullName.Replace((Get-Location).Path + "\", "").Replace("\", "/")
            }
        }
    }
    $responseFile = Join-Path (Get-Location) "build-clang.rsp"
    @(
        "-shared",
        "-o",
        "dist/sm64.dll",
        "-fno-strict-aliasing",
        "-fPIC",
        "-fvisibility=hidden",
        "-DSM64_LIB_EXPORT",
        "-DGBI_FLOATS",
        "-DVERSION_US",
        "-DNO_SEGMENTED_MEMORY",
        "-Isrc/decomp/include"
    ) + $files + @("-lm") | Set-Content -Encoding ASCII $responseFile

    & $clang.FullName "@$responseFile"
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    Copy-Item "src\libsm64.h" "dist\include\libsm64.h" -Force
}
Pop-Location

if (-not (Test-Path "$InstallDir\dist\sm64.dll")) {
    throw "libsm64 build finished but dist/sm64.dll was not found"
}

Write-Host "Built $InstallDir/dist/sm64.dll"
Write-Host "Restart sm64_physics_sandbox.exe; it will use the libsm64 backend automatically."
