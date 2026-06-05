[CmdletBinding()]
param(
    [string]$Preset = "debug",
    [switch]$SkipFormat,
    [switch]$Tidy
)

$ErrorActionPreference = "Stop"

function Invoke-FormatCheck {
    if ($SkipFormat) {
        Write-Host "Skipping clang-format."
        return
    }

    if (-not (Get-Command clang-format -ErrorAction SilentlyContinue)) {
        Write-Host "Skipping clang-format: command not found."
        return
    }

    $roots = @("include", "src", "tests") | Where-Object { Test-Path $_ }
    if ($roots.Count -eq 0) {
        return
    }

    $files = Get-ChildItem -Path $roots -Recurse -File -Include *.c, *.h |
        Where-Object {
            $relative = Resolve-Path -Relative $_.FullName
            $relative -notlike ".\include\glad\*" -and
                $relative -notlike ".\include\KHR\*" -and
                $relative -notlike ".\include\nuklear\*" -and
                $relative -ne ".\src\glad.c"
        } |
        Sort-Object FullName

    if ($files.Count -eq 0) {
        return
    }

    & clang-format --dry-run --Werror @($files.FullName)
}

function Invoke-Tidy {
    if (-not $Tidy) {
        return
    }

    if (-not (Get-Command run-clang-tidy -ErrorAction SilentlyContinue)) {
        Write-Host "Skipping clang-tidy: run-clang-tidy command not found."
        return
    }

    & run-clang-tidy -p "build/$Preset"
}

Invoke-FormatCheck
cmake --preset $Preset
cmake --build --preset $Preset --parallel
ctest --preset $Preset --output-on-failure
Invoke-Tidy
