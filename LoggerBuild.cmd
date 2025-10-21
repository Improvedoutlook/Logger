@echo off
REM Simple wrapper to build Logger as GUI using the PowerShell build helper
REM Usage: LoggerBuild  (from project folder or include folder in PATH)

powershell -NoProfile -ExecutionPolicy Bypass -Command "& './build.ps1' -Output 'Logger.exe' -Gui"
