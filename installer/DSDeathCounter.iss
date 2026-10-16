; Inno Setup 6 script (https://jrsoftware.org/isinfo.php) - per-user install, no admin rights.
; Build build\DSDeathCounter.exe first (build.bat), then: ISCC.exe installer\DSDeathCounter.iss
; The version comes from the exe resources (src\version.h); pass /DAppVersion=x.y.z to override.

#define AppName "DS Death Counter"
#define AppExe "DSDeathCounter.exe"
#define AppExePath AddBackslash(SourcePath) + "..\build\" + AppExe
#define AppURL "https://github.com/Theuzin69/ds-death-counter"
#ifndef AppVersion
  #define AppVersion GetStringFileInfo(AppExePath, "ProductVersion")
#endif

[Setup]
AppId={{6C2F3E1B-8F4D-4C55-9C1E-5A0B7D2E9F31}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=MatheusMuzzi
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
DefaultDirName={localappdata}\Programs\DSDeathCounter
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=..\dist
OutputBaseFilename=DSDeathCounter-Setup-{#AppVersion}
SetupIconFile=..\res\app.ico
UninstallDisplayIcon={app}\{#AppExe}
LicenseFile=..\LICENSE
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#ifndef NoAppMutex
AppMutex=DSDeathCounter_SingleInstance
#endif

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
brazilianportuguese.AutoStart=Iniciar com o Windows (minimizado na bandeja)
english.AutoStart=Start with Windows (minimized to the tray)
brazilianportuguese.Options=Opções:
english.Options=Options:

[Tasks]
Name: "autostart"; Description: "{cm:AutoStart}"; GroupDescription: "{cm:Options}"
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#AppExePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; Same value the app writes itself when "Iniciar com o Windows" is toggled.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "DSDeathCounter"; ValueData: """{app}\{#AppExe}"" --tray"; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Settings and death history live in %APPDATA%\DSDeathCounter and are kept on purpose.
Type: dirifempty; Name: "{app}"
