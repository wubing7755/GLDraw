param(
    [string]$Version = "",
    [string]$Generator = "MinGW Makefiles",
    [string]$BuildDir = "build/Release",
    [string]$DistDir = "dist",
    [string]$Platform = "",
    [switch]$SkipBuild,
    [switch]$SkipRuntimeDlls
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $RepoRoot

function Get-ProjectVersion {
    $cmakeText = Get-Content "CMakeLists.txt" -Raw
    $match = [regex]::Match($cmakeText, "project\s*\(\s*GLDraw[\s\S]*?VERSION\s+([0-9A-Za-z._-]+)")
    if (-not $match.Success) {
        throw "Could not find project version in CMakeLists.txt."
    }
    return $match.Groups[1].Value
}

function Get-DefaultPlatform {
    if ($IsWindows -or $env:OS -eq "Windows_NT") {
        return "windows-x64"
    }
    if ($IsMacOS) {
        return "macos"
    }
    return "linux-x64"
}

function Copy-IfExists {
    param(
        [string]$Path,
        [string]$Destination
    )

    if (Test-Path $Path) {
        Copy-Item -LiteralPath $Path -Destination $Destination -Force
    }
}

function Get-ObjdumpDllNames {
    param([string]$Executable)

    $objdump = Get-Command objdump -ErrorAction SilentlyContinue
    if (-not $objdump) {
        Write-Host "objdump not found; skipping runtime DLL scan."
        return @()
    }

    $output = & $objdump.Source -p $Executable
    if ($LASTEXITCODE -ne 0) {
        Write-Host "objdump failed; skipping runtime DLL scan."
        return @()
    }

    return $output |
        Select-String "DLL Name:\s+(.+)$" |
        ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() } |
        Sort-Object -Unique
}

function Copy-RuntimeDlls {
    param([string]$Executable)

    if (-not ($IsWindows -or $env:OS -eq "Windows_NT")) {
        return
    }

    $systemDlls = @(
        "advapi32.dll",
        "comdlg32.dll",
        "gdi32.dll",
        "kernel32.dll",
        "msvcrt.dll",
        "ole32.dll",
        "opengl32.dll",
        "shell32.dll",
        "ucrtbase.dll",
        "user32.dll",
        "winmm.dll"
    )

    $exeDir = Split-Path -Parent $Executable
    $dllNames = Get-ObjdumpDllNames $Executable

    foreach ($dllName in $dllNames) {
        $normalizedDllName = $dllName.ToLowerInvariant()
        if ($systemDlls -contains $normalizedDllName) {
            continue
        }

        if ($normalizedDllName.StartsWith("api-ms-win-") -or $normalizedDllName.StartsWith("ext-ms-")) {
            continue
        }

        $found = Get-Command $dllName -ErrorAction SilentlyContinue
        if ($found -and (Test-Path $found.Source)) {
            Copy-Item -LiteralPath $found.Source -Destination $exeDir -Force
            Write-Host "Bundled runtime DLL: $dllName"
        } else {
            Write-Host "Runtime DLL not found on PATH: $dllName"
        }
    }
}

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Get-ProjectVersion
}

if ([string]::IsNullOrWhiteSpace($Platform)) {
    $Platform = Get-DefaultPlatform
}

$BuildDir = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $BuildDir))
$DistDir = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $DistDir))
$StageRoot = Join-Path $DistDir "stage"
$PackageName = "GLDraw-v$Version-$Platform"
$PackageRoot = Join-Path $StageRoot $PackageName
$ArchivePath = Join-Path $DistDir "$PackageName.zip"

cmake -G $Generator -DCMAKE_BUILD_TYPE=Release -S $RepoRoot -B $BuildDir
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

if (-not $SkipBuild) {
    cmake --build $BuildDir --parallel
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }
}

if (Test-Path $StageRoot) {
    Remove-Item -LiteralPath $StageRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $PackageRoot | Out-Null

cmake --install $BuildDir --prefix $PackageRoot
if ($LASTEXITCODE -ne 0) { throw "CMake install failed." }

Copy-IfExists "LICENSE.txt" $PackageRoot
Copy-IfExists "README.md" $PackageRoot
Copy-IfExists "README.zh-CN.md" $PackageRoot
Copy-IfExists "doc/build.md" $PackageRoot
Copy-IfExists "doc/controls.md" $PackageRoot

$exePath = Join-Path $PackageRoot "bin/GLDraw.exe"
if ((Test-Path $exePath) -and -not $SkipRuntimeDlls) {
    Copy-RuntimeDlls $exePath
}

if (Test-Path $ArchivePath) {
    Remove-Item -LiteralPath $ArchivePath -Force
}

Compress-Archive -Path $PackageRoot -DestinationPath $ArchivePath -Force

Write-Host ""
Write-Host "Release package created:"
Write-Host "  $ArchivePath"
