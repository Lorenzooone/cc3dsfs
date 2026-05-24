set BUILD_PLATFORM=%1
set BUILD_TYPE=%2
set LIBDWI_PATCH_FOLDER_PATH=%3
set OUT_FOLDER=%4

REM Remove forwards slashes for copy...
set "LIBDWI_PATCH_FOLDER_PATH=%LIBDWI_PATCH_FOLDER_PATH:/=\%"
set "OUT_FOLDER=%OUT_FOLDER:/=\%"

set WDK_URL=https://go.microsoft.com/fwlink/p/?LinkID=253170
set VSWHERE_URL=https://github.com/microsoft/vswhere/releases/download/3.1.7/vswhere.exe
set LIBDWI_URL=https://github.com/pbatard/libwdi.git

if not exist "wdk-redist.msi" (
curl -L %WDK_URL% -o wdk-redist.msi
)
msiexec /a wdk-redist.msi /qn TARGETDIR=%CD%\wdk

git clone %LIBDWI_URL%
set BUILD_MACROS="WDK_DIR=\"../../wdk/Windows Kits/8.0\""
xcopy /s /y "%LIBDWI_PATCH_FOLDER_PATH%\\to_copy\\" ".\\libwdi\\"
cd libwdi

if not exist "vswhere.exe" (
curl -L %VSWHERE_URL% -o vswhere.exe
)

setlocal enabledelayedexpansion

for /f "usebackq tokens=*" %%i in (`vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
  set MSBUILD_EXE="%%i"
)

@RD /S /Q ".\\%BUILD_PLATFORM%"

REM Needed for embedder... For now, fixed to x64...
%MSBUILD_EXE% ./libwdi.sln /m /p:Configuration=%BUILD_TYPE%,Platform=x64,BuildMacros=%BUILD_MACROS%

%MSBUILD_EXE% ./libwdi.sln /m /p:Configuration=%BUILD_TYPE%,Platform=%BUILD_PLATFORM%,BuildMacros=%BUILD_MACROS%

copy ".\\%BUILD_PLATFORM%\\%BUILD_TYPE%\\examples\\wdi-simple.exe" "%OUT_FOLDER%\wdi-simple.exe"
