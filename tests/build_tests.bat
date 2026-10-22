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
set GAMESRC=src\game_reader.cpp src\ds1_data.cpp src\games\*.cpp

cl %FLAGS% tests\scan_test.cpp %GAMESRC% /Fe:build\tests\scan_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\bb_scan_test.cpp %GAMESRC% /Fe:build\tests\bb_scan_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\bb_live_test.cpp %GAMESRC% /Fe:build\tests\bb_live_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
rem Stand-in for the Bloodborne port (same process name) to test the live reader without the game.
if not exist build\tests\fake_port mkdir build\tests\fake_port
cl %FLAGS% tests\bb_fake_port.cpp %GAMESRC% /Fe:build\tests\fake_port\bbport.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\hk_mono_test.cpp %GAMESRC% /Fe:build\tests\hk_mono_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\hk_live_test.cpp %GAMESRC% /Fe:build\tests\hk_live_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\hk_death_test.cpp %GAMESRC% /Fe:build\tests\hk_death_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\boss_cause_test.cpp src\ds1_data.cpp /Fe:build\tests\boss_cause_test.exe /link /SUBSYSTEM:CONSOLE || exit /b 1
cl %FLAGS% tests\overlay_test.cpp src\overlay.cpp src\util.cpp /Fe:build\tests\overlay_test.exe ^
   /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib ole32.lib || exit /b 1
echo Testes compilados em build\tests\
