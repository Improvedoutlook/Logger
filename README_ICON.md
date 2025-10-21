# Logger Icon

Note: The text area now supports Ctrl+A to select all text for easy copying (implemented by subclassing the edit control and handling Ctrl+A to send EM_SETSEL).

# Embedding a custom icon into the Logger executable

If you have a `Logger_icon.ico` file in the project root and want the EXE to show that icon (instead of the generic one), embed the icon as a Windows resource and link it into the final executable.

Files added to this repo to help:

- `Logger.rc` - simple resource script that references `Logger_icon.ico`.
- `build.ps1` - PowerShell helper that uses `windres` + `gcc` (MinGW/MSYS2) to build `main.c` and embed the icon.

Quick steps (MinGW / MSYS2):

1. Install MSYS2 (https://www.msys2.org/) and install the packages `mingw-w64-x86_64-gcc` and `mingw-w64-x86_64-binutils` (which provide `gcc` and `windres`).
2. Open an MSYS2 MinGW shell or ensure `gcc` and `windres` are in your PATH.
3. From the project root (where `main.c`, `Logger.rc`, and `Logger_icon.ico` live) run PowerShell and execute:

```powershell
./build.ps1
```

This will produce `Logger.exe` with the embedded icon.

Visual Studio steps:

1. In Solution Explorer, right-click the project -> Add -> Existing Item... -> select `Logger.rc` (and ensure `Logger_icon.ico` is also in the project folder).
2. Open the Project Properties -> Linker/Manifest or Resources and set the application icon, or use the Resources view to mark the icon as the application's icon.

Troubleshooting / Notes:

- Explorer caches file icons. If the EXE still shows the old icon after rebuilding, clear the icon cache or create a new shortcut to the EXE and refresh Explorer. Log out/log in or reboot if necessary.
- If you use a shortcut (on Desktop or Start Menu), Windows will use the icon embedded in the EXE unless the shortcut explicitly has a different icon set.
- Make sure your `.ico` file contains Windows-friendly sizes (16x16, 32x32, 48x48, 256x256). Many icon designers/exporters produce multi-size `.ico` files.
 
Quick short build commands

Two tiny wrappers are included for convenience:

- `LoggerBuild.cmd` — run this from Explorer or CMD/PowerShell in the project folder to build `Logger.exe` as a GUI app.
- `LoggerBuild.ps1` — PowerShell wrapper that calls `build.ps1` with `-Gui`.

Example (PowerShell):

```powershell
.\LoggerBuild.ps1
```

Or (CMD or PowerShell):

```powershell
.\LoggerBuild.cmd
```

Console (debug) build wrappers

If you want a console build (so the app opens a terminal window), use the `LoggerBuildC` wrappers which build `Logger_console.exe`:

```powershell
.\LoggerBuildC.ps1
.\LoggerBuildC.cmd
```



