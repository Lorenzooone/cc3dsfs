@echo off
setlocal enabledelayedexpansion

:: Usage:
:: driver_remover.bat originalname.inf
::
:: Example:
:: driver_remover.bat cyusb3.inf
:: driver_remover.bat nstd_usb.inf
:: driver_remover.bat nstd_usb_new.inf
:: driver_remover.bat intraoralsensor.inf

if "%~1"=="" (
    echo Usage: %~nx0 OriginalDriverName.inf
    exit /b 1
)



set TARGET=%~1
set FOUND=
set PREVIOUS=

for /f "delims=" %%L in ('pnputil /enum-drivers') do (

    set LINE=%%L


    :: Split on first colon
    for /f "tokens=1,* delims=:" %%A in ("!LINE!") do (

        set VALUE=%%B

        :: Trim leading spaces
        for /f "tokens=* delims= " %%X in ("!VALUE!") do set VALUE=%%X

        :: Second field = original INF name
        if /I "!VALUE!"=="%TARGET%" (
            set "FOUND=!PREVIOUS!"
        )
        set PREVIOUS=!VALUE!
    )
)

if "%FOUND%"=="" (
    echo Driver not found.
    exit /b 1
)

echo Found: %FOUND%
echo Removing driver...

pnputil /delete-driver %FOUND% /uninstall /force

if errorlevel 1 (
    echo Failed to remove driver.
    exit /b 1
)

echo Driver removed successfully.