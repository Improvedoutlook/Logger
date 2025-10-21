# Short wrapper to build Logger_console.exe (console build)
param()

& "$PSScriptRoot\build.ps1" -Output "Logger_console.exe"

# Start the console executable so a terminal window opens
Start-Process -FilePath "$PSScriptRoot\Logger_console.exe" -NoNewWindow:$false
