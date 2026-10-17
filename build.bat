@echo off
rem Builds build\DSDeathCounter.exe with the Visual Studio C++ toolchain (2019 or newer).
setlocal
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo Visual Studio nao encontrado. Instale o "Desktop development with C++" ou as Build Tools.
    exit /b 1
)
set "VSINSTALL="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (
    echo Ferramentas C++ do Visual Studio nao encontradas.
    exit /b 1
)
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 || exit /b 1

if not exist build mkdir build
if not exist build\obj mkdir build\obj

rc /nologo /fo build\obj\app.res res\app.rc || exit /b 1

set IMGUI=third_party\imgui
cl /nologo /std:c++20 /O2 /MT /EHsc /utf-8 /W3 /DNDEBUG /DUNICODE /D_UNICODE /DNOMINMAX ^
   /I src /I %IMGUI% /I %IMGUI%\backends /I third_party ^
   src\*.cpp src\games\*.cpp ^
   %IMGUI%\imgui.cpp %IMGUI%\imgui_draw.cpp %IMGUI%\imgui_tables.cpp %IMGUI%\imgui_widgets.cpp ^
   %IMGUI%\backends\imgui_impl_win32.cpp %IMGUI%\backends\imgui_impl_dx11.cpp %IMGUI%\misc\cpp\imgui_stdlib.cpp ^
   build\obj\app.res ^
   /Fobuild\obj\ /Fe:build\DSDeathCounter.exe ^
   /link /SUBSYSTEM:WINDOWS /MANIFEST:NO ^
   user32.lib gdi32.lib shell32.lib advapi32.lib comdlg32.lib ole32.lib dwmapi.lib version.lib || exit /b 1

echo.
echo Pronto: build\DSDeathCounter.exe
