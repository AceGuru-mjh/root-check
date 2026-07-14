# APEX-Root liboqs Android Build Script (PowerShell)
# Cross-compiles liboqs v0.15.0 for arm64-v8a using Android NDK
# Usage: .\scripts\build_liboqs_android.ps1 [-NDKPath C:\android-ndk-r26b]

param(
    [string]$NDKPath = "",
    [string]$LibOqsVersion = "0.15.0"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$LibOqsDir = Join-Path $ProjectRoot "app\src\main\cpp\liboqs"
$BuildDir = Join-Path $ProjectRoot "build\liboqs_android"

Write-Host "=== APEX-Root liboqs Android Build ===" -ForegroundColor Cyan
Write-Host "liboqs version: $LibOqsVersion"
Write-Host "Target: arm64-v8a"
Write-Host ""

# Find NDK
if (-not $NDKPath) {
    $possible = @(
        "${env:ANDROID_NDK_HOME}",
        "${env:ANDROID_HOME}\ndk-bundle",
        "${env:ANDROID_HOME}\ndk\26.3.11579264",
        "${env:LOCALAPPDATA}\Android\Sdk\ndk\26.3.11579264",
        "${env:LOCALAPPDATA}\Android\Sdk\ndk-bundle"
    )
    foreach ($p in $possible) {
        if (Test-Path "$p\build\cmake\android.toolchain.cmake") {
            $NDKPath = $p
            break
        }
    }
}

if (-not $NDKPath -or -not (Test-Path $NDKPath)) {
    Write-Host "ERROR: Android NDK not found. Set NDKPath or ANDROID_NDK_HOME." -ForegroundColor Red
    exit 1
}
Write-Host "Using NDK: $NDKPath"

# Download liboqs if not present
if (-not (Test-Path "$LibOqsDir\CMakeLists.txt")) {
    Write-Host "Downloading liboqs v$LibOqsVersion..."
    $url = "https://github.com/open-quantum-safe/liboqs/archive/refs/tags/$LibOqsVersion.tar.gz"
    $tarball = Join-Path $env:TEMP "liboqs-$LibOqsVersion.tar.gz"
    $extractDir = Join-Path $env:TEMP "liboqs-$LibOqsVersion"

    if (-not (Test-Path $tarball)) {
        Invoke-WebRequest -Uri $url -OutFile $tarball
    }

    if (Test-Path $extractDir) { Remove-Item -Recurse -Force $extractDir }
    tar -xzf $tarball -C $env:TEMP

    # Remove existing and copy
    if (Test-Path $LibOqsDir) { Remove-Item -Recurse -Force $LibOqsDir }
    Move-Item $extractDir $LibOqsDir
    Write-Host "liboqs extracted to $LibOqsDir"
} else {
    Write-Host "liboqs already present at $LibOqsDir"
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

# Configure with CMake
$toolchain = "$NDKPath\build\cmake\android.toolchain.cmake"
Write-Host "Configuring liboqs for Android arm64-v8a..."

cmake -S $LibOqsDir -B $BuildDir `
    -DCMAKE_TOOLCHAIN_FILE=$toolchain `
    -DANDROID_ABI=arm64-v8a `
    -DANDROID_PLATFORM=29 `
    -DANDROID_STL=c++_static `
    -DCMAKE_BUILD_TYPE=MinSizeRel `
    -DBUILD_SHARED_LIBS=OFF `
    -DOQS_BUILD_ONLY_LIB=ON `
    -DOQS_USE_OPENSSL=OFF `
    -DOQS_ENABLE_SIG_DILITHIUM=ON `
    -DOQS_ENABLE_SIG_ALL=OFF `
    -DOQS_ENABLE_KEM_ALL=OFF `
    -DOQS_OPT_FLAGS="-O2 -ffunction-sections -fdata-sections" `
    -DOQS_DIST_ARM=ON `
    -DOQS_DIST_ARM64=ON

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build
Write-Host "Building liboqs..."
cmake --build $BuildDir --target oqs -- -j$env:NUMBER_OF_PROCESSORS

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

# Install headers + lib to liboqs directory
$LibDir = "$LibOqsDir\lib"
$IncludeDir = "$LibOqsDir\include"
if (-not (Test-Path $LibDir)) { New-Item -ItemType Directory -Path $LibDir -Force | Out-Null }
if (-not (Test-Path $IncludeDir)) { New-Item -ItemType Directory -Path $IncludeDir -Force | Out-Null }

Copy-Item "$BuildDir\lib\liboqs.a" "$LibDir\liboqs.a" -Force
Copy-Item "$BuildDir\include\oqs\*.h" "$IncludeDir\" -Force
Copy-Item "$LibOqsDir\src\common\common.h" "$IncludeDir\oqs\" -Force

Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Green
Write-Host "Library: $LibDir\liboqs.a"
Write-Host "Headers: $IncludeDir\"
Write-Host ""
Write-Host "liboqs is ready. Run 'gradlew assembleRelease' to build APEX-Root with PQ crypto."
