@echo off

:: BatchGotAdmin
:-------------------------------------
REM  --> Check for permissions
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    set params = %*:"=""
    echo UAC.ShellExecute "cmd.exe", "/c %~s0 %params%", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
:--------------------------------------


call "..\subscripts\driver_uninstaller_currently_connected.bat" 04B4 8613
call "..\subscripts\driver_uninstaller_currently_connected.bat" 0752 8613
call "..\subscripts\driver_uninstaller_currently_connected.bat" 04B4 1004

call "..\subscripts\driver_remover_vid_pid_based.bat" 04B4 8613
call "..\subscripts\driver_remover_vid_pid_based.bat" 0752 8613
call "..\subscripts\driver_remover_vid_pid_based.bat" 04B4 1004

call "..\subscripts\driver_remover_name_based.bat" cyusb3.inf
