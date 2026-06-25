# APEX-Root Module Build Script (PowerShell)
# Builds the Magisk/KSU/APatch flashable ZIP

$ErrorActionPreference = "Stop"
$ROOT = Split-Path -Parent $PSScriptRoot
$OUT = "$ROOT\build\output"
$MODULE_DIR = "$ROOT\module"
$MODULE_OUT = "$OUT\apex-root"

Write-Host "=== APEX-Root Module Builder ===" -ForegroundColor Cyan

# Clean
if (Test-Path $MODULE_OUT) { Remove-Item -Recurse -Force $MODULE_OUT }
New-Item -ItemType Directory -Force -Path $MODULE_OUT | Out-Null

# Copy module files
Write-Host "[1/5] Copying module files..."
Copy-Item "$MODULE_DIR\*" -Destination $MODULE_OUT -Recurse

# Copy SELinux policy
Write-Host "[2/5] Copying SELinux policy..."
Copy-Item "$ROOT\sepolicy\sepolicy.rule" -Destination $MODULE_OUT

# Copy BPF programs (prebuilt)
Write-Host "[3/5] Copying BPF programs..."
$BPF_OUT="$MODULE_OUT\bpf"
New-Item -ItemType Directory -Force -Path $BPF_OUT | Out-Null
if (Test-Path "$ROOT\build\bpf\apex_firewall.bpf.o") {
    Copy-Item "$ROOT\build\bpf\*.o" -Destination $BPF_OUT
} else {
    Write-Host "[WARN] BPF .o files not found, building stub" -ForegroundColor Yellow
    Set-Content -Path "$BPF_OUT\apex_firewall.bpf.o" -Value $null
}

# Copy native libraries (prebuilt)
Write-Host "[4/5] Copying native libraries..."
$LIB_OUT="$MODULE_OUT\lib"
New-Item -ItemType Directory -Force -Path "$LIB_OUT\arm64-v8a" | Out-Null
New-Item -ItemType Directory -Force -Path "$LIB_OUT\armeabi-v7a" | Out-Null
if (Test-Path "$ROOT\build\libs") {
    Copy-Item "$ROOT\build\libs\*" -Destination "$LIB_OUT" -Recurse
} else {
    Write-Host "[WARN] Native libs not found" -ForegroundColor Yellow
}

# Create ZIP
Write-Host "[5/5] Creating flashable ZIP..."
$ZIP_NAME = "apex-root-v1.0.zip"
$ZIP_PATH = "$OUT\$ZIP_NAME"
if (Test-Path $ZIP_PATH) { Remove-Item -Force $ZIP_PATH }

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($MODULE_OUT, $ZIP_PATH)

Write-Host "=== Done ===" -ForegroundColor Cyan
Write-Host "Module ZIP: $ZIP_PATH"
Write-Host "Size: $((Get-Item $ZIP_PATH).Length / 1KB) KB"
