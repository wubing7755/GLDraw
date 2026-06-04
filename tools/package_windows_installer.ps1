param(
    [string]$Version = "",
    [string]$Platform = "windows-x64",
    [string]$DistDir = "dist",
    [string]$SourceDir = "",
    [string]$OutputDir = "",
    [string]$OutputBaseFilename = ""
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

if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Get-ProjectVersion
}

$DistDir = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $DistDir))

if ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $SourceDir = Join-Path $DistDir "stage/GLDraw-v$Version-$Platform"
}

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = $DistDir
}

if ([string]::IsNullOrWhiteSpace($OutputBaseFilename)) {
    $OutputBaseFilename = "GLDraw-v$Version-$Platform-setup"
}

$SourceDir = [System.IO.Path]::GetFullPath($SourceDir)
$OutputDir = [System.IO.Path]::GetFullPath($OutputDir)
$SetupScript = Join-Path $RepoRoot "packaging/windows/GLDraw.iss"

if (-not (Test-Path (Join-Path $SourceDir "bin/GLDraw.exe"))) {
    throw "Missing staged GLDraw executable under $SourceDir."
}

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

$iscc = Get-Command iscc -ErrorAction SilentlyContinue
if (-not $iscc) {
    $isccPath = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    if (-not (Test-Path $isccPath)) {
        throw "ISCC.exe not found. Install Inno Setup 6 or add iscc to PATH."
    }
} else {
    $isccPath = $iscc.Source
}

& $isccPath `
    $SetupScript `
    "/DAppVersion=$Version" `
    "/DSourceDir=$SourceDir" `
    "/DOutputDir=$OutputDir" `
    "/DOutputBaseFilename=$OutputBaseFilename"

if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed."
}

$InstallerPath = Join-Path $OutputDir "$OutputBaseFilename.exe"

Write-Host ""
Write-Host "Windows installer created:"
Write-Host "  $InstallerPath"
