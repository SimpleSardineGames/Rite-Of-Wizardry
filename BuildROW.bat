@echo off
REM Builds the ROWDEVBUILD editor module without Visual Studio.
REM Put this file in the same folder as ROWDEVBUILD.uproject, close the Unreal Editor, then double-click it.

set "UE_BUILD=C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat"
set "PROJECT=%~dp0ROWDEVBUILD.uproject"

if not exist "%UE_BUILD%" (
    echo Could not find Unreal's Build.bat at:
    echo   %UE_BUILD%
    echo Edit the UE_BUILD line in this file to match your Unreal Engine 5.8 install.
    pause
    exit /b 1
)

if not exist "%PROJECT%" (
    echo Could not find ROWDEVBUILD.uproject next to this file.
    echo Move this .bat into the same folder as ROWDEVBUILD.uproject.
    pause
    exit /b 1
)

call "%UE_BUILD%" ROWDEVBUILDEditor Win64 Development -Project="%PROJECT%" -WaitMutex

echo.
if %ERRORLEVEL% EQU 0 (
    echo ===== BUILD SUCCEEDED - you can open the project now =====
) else (
    echo ===== BUILD FAILED - scroll up to the first line containing "error" =====
)
pause