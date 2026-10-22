; Inno Setup 6 script for ABC3D.
; Built by tools\package.ps1, which stages the files and passes AppVersion/SourceDir.
; Standalone: ISCC.exe /DAppVersion=1.0.0 installer\ABC3D.iss  (after staging out\package\ABC3D)

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\out\package\ABC3D"
#endif

#define AppName "ABC3D"
#define AppExe "ABC3D.exe"

[Setup]
; AppId identifies the app for upgrades/uninstall. Never change it between releases.
AppId={{CE2E521F-233A-4537-A88A-ECC8105A7FB1}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=ABC3D Team
AppPublisherURL=https://abc3d.me
AppSupportURL=https://abc3d.me
VersionInfoVersion={#AppVersion}

; Per-user install by default: no UAC prompt, lands in %LocalAppData%\Programs\ABC3D.
; Users can still choose "Install for all users" (Program Files) from the dialog.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

OutputDir=..\out\installer
OutputBaseFilename=ABC3D-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
; Lets an upgrade replace files while ABC3D is open
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
; "Import Tutorial" copies files into assets\tutorials. Keep it writable when installed
; for all users under Program Files.
Name: "{app}\assets\tutorials"; Permissions: users-modify

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent
