<#
Simple build helper for Windows that embeds Logger_icon.ico into the final EXE.

Two common approaches shown:
- MinGW (windres + gcc) — lightweight, works from MSYS2/MinGW installs.
- Visual Studio (cl/link) — instructions only (uncomment to use).

Run from project root (where main.c and Logger_icon.ico live):
    .\build.ps1
.\Logger = normal build/run
.\Logger -Gui = GUI build/run
#>

param(
    [ValidateNotNullOrEmpty()][string]$Output = "Logger.exe",
    [ValidateNotNullOrEmpty()][string]$Source = "main.c",
    [ValidateNotNullOrEmpty()][string]$Resource = "Logger.rc",
    [switch]$Gui
)
function Invoke-BuildWithMinGW {
    param()

    Write-Host "Building with MinGW (windres + gcc)..." -ForegroundColor Cyan

    # Validate source/resource files
    if (-not (Test-Path -Path $Resource)) {
        throw "Resource file '$Resource' not found."
    }
    if (-not (Test-Path -Path $Source)) {
        throw "Source file '$Source' not found."
    }

    # Locate tools in PATH
    $windresCmd = Get-Command windres -ErrorAction SilentlyContinue
    if (-not $windresCmd) {
        throw "windres not found in PATH. Install mingw-w64 or MSYS2 and ensure windres is available."
    }
    $gccCmd = Get-Command gcc -ErrorAction SilentlyContinue
    if (-not $gccCmd) {
        throw "gcc not found in PATH. Install mingw-w64 or MSYS2 and ensure gcc is available."
    }

    # Compile the resource script into a .res file
    $resFile = "Logger.res"
    & $windresCmd.Path $Resource -O coff -o $resFile
    if ($LASTEXITCODE -ne 0) { throw "windres failed with exit code $LASTEXITCODE" }

    # Compile and link the program with the resource
    $gccArgs = @($Source, "spellchecker.c", $resFile, '-o', $Output)
    if ($Gui) { $gccArgs += '-mwindows' }

    & $gccCmd.Path @gccArgs
    if ($LASTEXITCODE -ne 0) { throw "gcc failed with exit code $LASTEXITCODE" }

    Write-Host "Built $Output successfully." -ForegroundColor Green
}

function Show-VisualStudioInstructions {
    Write-Host "If you're using Visual Studio, add the .rc to your project and set the icon as the application's icon via Project Properties -> Linker -> Manifest -> Icon or via the Resources view." -ForegroundColor Cyan
    Write-Host "From the Developer Command Prompt you could also run: rc.exe $Resource && cl /Fe:$Output $Source Logger.res" -ForegroundColor Cyan
}

# Main
try {
    Invoke-BuildWithMinGW
} catch {
    Write-Host "Build failed: $_" -ForegroundColor Red
    exit 1
}
