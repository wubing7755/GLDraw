#define AppName "GLDraw"
#ifndef AppVersion
#define AppVersion "0.0.0"
#endif
#ifndef SourceDir
#define SourceDir "..\..\dist\stage\GLDraw-v" + AppVersion + "-windows-x64"
#endif
#ifndef OutputDir
#define OutputDir "..\..\dist"
#endif
#ifndef OutputBaseFilename
#define OutputBaseFilename "GLDraw-v" + AppVersion + "-windows-x64-setup"
#endif

[Setup]
AppId={{B28BA2D6-2B20-40EC-B3D7-29F5BDE5DE2F}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=GLDraw
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile={#SourceDir}\LICENSE.txt
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\bin\GLDraw.exe
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GLDraw"; Filename: "{app}\bin\GLDraw.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\GLDraw"; Filename: "{app}\bin\GLDraw.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\GLDraw.exe"; Description: "{cm:LaunchProgram,GLDraw}"; Flags: nowait postinstall skipifsilent
