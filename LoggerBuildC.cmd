
@echo off
REM Wrapper to build Logger_console.exe as a console application and then launch it
REM Usage: LoggerBuildC

powershell -NoProfile -ExecutionPolicy Bypass -Command "& './build.ps1' -Output 'Logger_console.exe'"

REM Start the built executable in a new console window
start "" "%~dp0Logger_console.exe"
