@echo off
rem Builds xinput9_1_0.dll with the Visual Studio 2022 x64 toolchain.
rem Usage: build.cmd [x64-release|x64-debug]   (default: x64-release)
setlocal
set PRESET=%~1
if "%PRESET%"=="" set PRESET=x64-release

rem vcvars64.bat needs reg.exe/where.exe/vswhere.exe to locate the Windows SDK.
set "PATH=%SystemRoot%\System32;%SystemRoot%;%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSDIR=%%i
if not defined VSDIR (
    echo Visual Studio with the C++ x64 toolchain was not found.
    exit /b 1
)
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

cd /d "%~dp0"
cmake --preset %PRESET% || exit /b 1
cmake --build --preset %PRESET% || exit /b 1
echo.
echo Built: %~dp0build\%PRESET%\xinput9_1_0.dll
