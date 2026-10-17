@echo off
rem Builds the small test programs into build\tests\.
setlocal
cd /d "%~dp0.."
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSINSTALL="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL exit /b 1
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1
if not exist build\tests\obj mkdir build\tests\obj

set FLAGS=/nologo /std:c++20 /O2 /MT /EHsc /utf-8 /W3 /DUNICODE /D_UNICODE /DNOMINMAX /I src /Fobuild\tests\obj\

cl %FLAGS% tests\scan_test.cpp src\game_reader.cpp src\games\*.cpp /Fe:build\tests\scan_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\overlay_test.cpp src\overlay.cpp src\util.cpp /Fe:build\tests\overlay_test.exe ^
   /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib ole32.lib || exit /b 1
echo Testes compilados em build\tests\
