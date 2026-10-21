<#
.SYNOPSIS
    Builds ABC3D (Release|x64), stages a self-contained folder, and compiles the installer.

.DESCRIPTION
    Output:
      out\package\ABC3D\              portable folder (exe + Qt DLLs + MSVC runtime + assets)
      out\installer\ABC3D-Setup-<v>.exe   Inno Setup installer (if ISCC.exe is found)

    Run from anywhere:  powershell -ExecutionPolicy Bypass -File tools\package.ps1

.PARAMETER Version
    Version stamped into the installer (and its file name).

.PARAMETER QtDir
    Qt kit used for both the build and windeployqt. Must match the kit the app is built with.

.PARAMETER SkipBuild
    Package the existing x64\Release\ABC3D.exe without rebuilding.

.PARAMETER SkipInstaller
    Stop after staging the folder (no Inno Setup step).
#>
param(
    [string]$Version = "1.0.0",
    [string]$QtDir = "C:\Qt\6.11.2\msvc2022_64",
    [switch]$SkipBuild,
    [switch]$SkipInstaller
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $root "out\package\ABC3D"
$exe = Join-Path $root "x64\Release\ABC3D.exe"

function Find-VsInstall {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) { throw "vswhere.exe not found; is Visual Studio installed?" }
    $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if (-not $path) { throw "No Visual Studio install with MSBuild found." }
    return $path
}

if (-not (Test-Path "$QtDir\bin\windeployqt.exe")) {
    throw "windeployqt.exe not found under $QtDir. Pass -QtDir <path to Qt kit>."
}

$vs = Find-VsInstall

# ---- 1. Build --------------------------------------------------------------
if (-not $SkipBuild) {
    Write-Host "==> Building Release|x64" -ForegroundColor Cyan
    $msbuild = Join-Path $vs "MSBuild\Current\Bin\MSBuild.exe"
    $msbuildArgs = @(
        (Join-Path $root "ABC3D.sln"),
        "/p:Configuration=Release", "/p:Platform=x64",
        "/p:QtInstall=$QtDir",
        "/m", "/nologo", "/v:minimal"
    )
    # The Qt VS Tools extension sets QtMsBuild inside the IDE; on the command line
    # point at the copy it installs under LocalAppData.
    if (-not $env:QtMsBuild -and (Test-Path "$env:LOCALAPPDATA\QtMsBuild\qt.targets")) {
        $msbuildArgs += "/p:QtMsBuild=$env:LOCALAPPDATA\QtMsBuild"
    }
    & $msbuild @msbuildArgs
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed ($LASTEXITCODE)." }
}

if (-not (Test-Path $exe)) { throw "$exe not found. Build Release|x64 first or drop -SkipBuild." }

# ---- 2. Stage --------------------------------------------------------------
Write-Host "==> Staging $stage" -ForegroundColor Cyan
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item $exe $stage

# Qt DLLs and plugins (platforms\qwindows.dll, styles, imageformats, iconengines for SVG, ...)
& "$QtDir\bin\windeployqt.exe" --release --no-translations --no-compiler-runtime (Join-Path $stage "ABC3D.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed ($LASTEXITCODE)." }

# MSVC runtime, deployed app-local so users need neither vc_redist nor admin rights
$crt = Get-ChildItem "$vs\VC\Redist\MSVC\*\x64\Microsoft.VC14*.CRT" -Directory |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $crt) { throw "MSVC x64 CRT redist folder not found under $vs\VC\Redist\MSVC." }
Copy-Item "$($crt.FullName)\*.dll" $stage

# Assets. Loaded via relative paths; main.cpp switches the working dir to the exe's folder.
$assets = Join-Path $root "assets"
$stageAssets = Join-Path $stage "assets"
foreach ($dir in "help documents", "icons", "shaders", "tutorials") {
    Copy-Item (Join-Path $assets $dir) (Join-Path $stageAssets $dir) -Recurse
}

# assets\models is mostly broken/degenerate test fixtures; ship only the clean samples.
$sampleModels = @(
    "Cube.stl", "box.obj", "triBox.obj", "beveledBox.obj", "ico_sphere.obj",
    "dodecahedron.obj", "dodecahedron.mtl", "utahTeapotBlinn.obj"
)
New-Item -ItemType Directory -Force (Join-Path $stageAssets "models") | Out-Null
foreach ($m in $sampleModels) {
    Copy-Item (Join-Path $assets "models\$m") (Join-Path $stageAssets "models")
}

# ---- 3. Installer ----------------------------------------------------------
if ($SkipInstaller) {
    Write-Host "==> Done (staged only): $stage" -ForegroundColor Green
    return
}

$iscc = @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe",
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $iscc) {
    Write-Warning "Inno Setup 6 not found; skipping installer. Install it with: winget install JRSoftware.InnoSetup"
    Write-Host "==> Staged folder: $stage" -ForegroundColor Green
    return
}

Write-Host "==> Compiling installer" -ForegroundColor Cyan
& $iscc "/DAppVersion=$Version" "/DSourceDir=$stage" (Join-Path $root "installer\ABC3D.iss")
if ($LASTEXITCODE -ne 0) { throw "ISCC failed ($LASTEXITCODE)." }

Write-Host "==> Installer: $(Join-Path $root "out\installer\ABC3D-Setup-$Version.exe")" -ForegroundColor Green
