param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [string]$QtRoot = $env:QTDIR
)

$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $PSScriptRoot
$sourceDir = Join-Path $projectDir 'external\qtgamepadlegacy'

if ([string]::IsNullOrWhiteSpace($QtRoot)) {
    $QtRoot = 'C:\Qt\6.11.2\msvc2022_64'
}

if (-not (Test-Path -LiteralPath "$sourceDir\CMakeLists.txt")) {
    throw "QtGamepadLegacy source was not found at $sourceDir. Clone with --recurse-submodules, or run: git submodule update --init --recursive"
}

if (-not (Test-Path -LiteralPath "$QtRoot\lib\cmake\Qt6\Qt6Config.cmake")) {
    throw "Qt 6.11.2 MSVC 2022 x64 was not found at $QtRoot. Install that Qt kit or set QTDIR to its root directory."
}

# Pick the generator for the newest Visual Studio that is installed, not just one CMake knows about.
$cmakeHelp = & cmake --help
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installedMajors = @()
if (Test-Path -LiteralPath $vswhere) {
    $installedMajors = & $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion |
        ForEach-Object { [int]($_.Split('.')[0]) }
}

$generator = $null
foreach ($candidate in @(@{ Major = 18; Name = 'Visual Studio 18 2026' }, @{ Major = 17; Name = 'Visual Studio 17 2022' })) {
    $installed = ($installedMajors.Count -eq 0) -or ($installedMajors -contains $candidate.Major)
    if ($installed -and ($cmakeHelp -match $candidate.Name)) {
        $generator = $candidate.Name
        break
    }
}
if (-not $generator) {
    throw 'CMake could not find a supported Visual Studio generator. Install Visual Studio with the Desktop development with C++ workload.'
}

$buildDir = Join-Path $sourceDir "out\build\vs-$Configuration"
$installDir = Join-Path $sourceDir "install\$Configuration"

cmake -S $sourceDir -B $buildDir -G $generator -A x64 -T v143 `
    "-DCMAKE_PREFIX_PATH=$QtRoot" `
    "-DCMAKE_INSTALL_PREFIX=$installDir"

cmake --build $buildDir --config $Configuration
cmake --install $buildDir --config $Configuration
