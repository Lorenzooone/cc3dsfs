@echo off
setlocal enabledelayedexpansion

:: Usage:
:: driver_remover.bat vid pid
::
:: For removing LibUSB-based drivers
::
:: Example:
:: driver_remover.bat 04B4 8613


if "%~2"=="" (
    echo Usage: %~nx0 VID PID
    exit /b 1
)

set VID=%~1
set PID=%~2

set TARGET=VID_%VID%^&PID_%PID%
set FOUND=
set PREVIOUS=
set PREVIOUS2=
set PREVIOUS3=
set PREVIOUS4=
set PREVIOUS5=
set PREVIOUS6=

for /f "delims=" %%L in ('pnputil /enum-drivers') do (

    set LINE=%%L


    :: Split on first colon
    for /f "tokens=1,* delims=:" %%A in ("!LINE!") do (

        set VALUE=%%B

        :: Trim leading spaces
        for /f "tokens=* delims= " %%X in ("!VALUE!") do set VALUE=%%X

        echo "!VALUE!" | find /I "%TARGET%" >nul

        :: Second field = original INF name
        if not errorlevel 1 (
            set "FOUND=!PREVIOUS6!"
        )
        set PREVIOUS6=!PREVIOUS5!
        set PREVIOUS5=!PREVIOUS4!
        set PREVIOUS4=!PREVIOUS3!
        set PREVIOUS3=!PREVIOUS2!
        set PREVIOUS2=!PREVIOUS!
        set PREVIOUS=!VALUE!
    )
)

if "%FOUND%"=="" (
    echo Driver not found.
    exit /b 2
)

echo Found: %FOUND%
echo Removing driver...

pnputil /delete-driver %FOUND% /uninstall /force

if errorlevel 1 (
    echo Failed to remove driver.
    exit /b 1
)

echo Driver removed successfully.
exit /b 0