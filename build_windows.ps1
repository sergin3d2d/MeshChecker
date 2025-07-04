# Windows Build Script for ApparelMeshChecker
#
# Prerequisites:
# 1. Visual Studio 2019 with C++ workload installed.
# 2. Qt 5.15.x for MSVC 2019 installed.
# 3. vcpkg installed and integrated.
#
# How to run:
# 1. Open a PowerShell terminal.
# 2. Navigate to the project root directory (where this script is located).
# 3. (First time only) Run: Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
# 4. Run the script: .\build_windows.ps1

# --- Configuration ---
# Adjust these paths to match your system configuration.

$ProjectName = "ApparelMeshChecker"
$QtPath = "P:\Projects\Exporter\Qt\6.9.1\msvc2022_64"
$VcpkgToolchainFile = "C:/Users/sergi/vcpkg/scripts/buildsystems/vcpkg.cmake"
$Generator = "Visual Studio 17 2022"
$Platform = "x64"
$BuildConfig = "Release"

# --- Script ---

# The build directory MUST be on a native Windows drive (e.g., C:, D:), not a WSL path.
$BuildDir = "C:\MeshChecker-build"
$SourceDir = $PSScriptRoot

Write-Host "--------------------------------------------------"
Write-Host "Starting Windows build for $ProjectName"
Write-Host "Source directory: $SourceDir"
Write-Host "Build directory:  $BuildDir"
Write-Host "--------------------------------------------------"

# Clean and create the build directory
if (Test-Path $BuildDir) {
    Write-Host "Removing existing build directory..."
    Remove-Item -Recurse -Force -Path $BuildDir
}
New-Item -ItemType Directory -Path $BuildDir | Out-Null

# 1. Run CMake to configure the project
Write-Host "`n[Step 1/4] Running CMake configuration..."
cmake -S $SourceDir -B $BuildDir -G "$Generator" -A $Platform -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchainFile" -DCMAKE_PREFIX_PATH="$QtPath"
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed!"
    exit 1
}

# 2. Build the project using CMake
Write-Host "`n[Step 2/4] Building the project ($BuildConfig)..."
cmake --build $BuildDir --config $BuildConfig
if ($LASTEXITCODE -ne 0) {
    Write-Error "Project build failed!"
    exit 1
}

# 3. Deploy Qt dependencies
Write-Host "`n[Step 3/4] Deploying Qt dependencies..."
$ExeOutputDir = "$BuildDir\$BuildConfig"
$ExePath = "$ExeOutputDir\$ProjectName.exe"
$WindeployqtPath = "$QtPath\bin\windeployqt.exe"

if (-not (Test-Path $ExePath)) {
    Write-Error "Executable not found at $ExePath. Cannot deploy Qt dependencies."
    exit 1
}

& $WindeployqtPath --release --no-translations $ExePath
if ($LASTEXITCODE -ne 0) {
    Write-Error "windeployqt failed!"
    exit 1
}

# 4. Create a distributable package using CPack
Write-Host "`n[Step 4/4] Creating distributable package..."
Set-Location $BuildDir
cpack -C $BuildConfig
if ($LASTEXITCODE -ne 0) {
    Write-Error "CPack packaging failed!"
    exit 1
}
Set-Location $SourceDir

Write-Host "`n--------------------------------------------------"
Write-Host "Build process completed successfully!"
Write-Host "- Executable and dependencies are in: $BuildDir\bin\$BuildConfig"
Write-Host "- Packaged ZIP file is in: $BuildDir"
Write-Host "--------------------------------------------------"
