<#
Simple build helper for Windows that embeds Logger_icon.ico into the final EXE.

Two common approaches shown:
- MinGW (windres + gcc) — lightweight, works from MSYS2/MinGW installs.
- Visual Studio (cl/link) — instructions only (uncomment to use).

Run from project root (where main.c and Logger_icon.ico live):
    .\build.ps1

#>

param(
    [string]$Output = "Logger.exe",
    [string]$Source = "main.c",
    [string]$Resource = "Logger.rc",
    [switch]$Gui
)

function Build-With-Mingw {
    Write-Host "Building with MinGW (windres + gcc)..."

    # Look for windres in PATH
    $windres = Get-Command windres -ErrorAction SilentlyContinue
    if (-not $windres) {
        Write-Host "windres not found in PATH. Install mingw-w64 or MSYS2 and ensure windres is available." -ForegroundColor Yellow
        return
    }

    # Compile the resource script into a .res file
    $resFile = "Logger.res"
    & windres.exe $Resource -O coff -o $resFile
    if ($LASTEXITCODE -ne 0) { throw "windres failed" }

    # Compile and link the program with the resource
    if ($Gui) {
        # -mwindows makes a GUI subsystem executable (no console)
        & gcc.exe $Source $resFile -o $Output -mwindows
    } else {
        & gcc.exe $Source $resFile -o $Output
    }
    if ($LASTEXITCODE -ne 0) { throw "gcc failed" }

    Write-Host "Built $Output successfully." -ForegroundColor Green
}

function Build-With-VisualStudio {
    Write-Host "If you're using Visual Studio, add the .rc to your project and set the icon as the application's icon via Project Properties -> Linker -> Manifest -> Icon or via the Resources view." -ForegroundColor Cyan
    Write-Host "From the Developer Command Prompt you could also run: rc.exe Logger.rc && cl /Fe:$Output main.c Logger.res" -ForegroundColor Cyan
}

# Main
try {
    Build-With-Mingw
} catch {
    Write-Host "Build failed: $_" -ForegroundColor Red
    exit 1
}
