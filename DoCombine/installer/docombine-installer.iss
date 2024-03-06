; Installer for DoCombine

[Setup]
AppName=DoCombine
AppVersion=1.0.0
WizardStyle=modern
OutputBaseFilename=InstallDoCombine
PrivilegesRequiredOverridesAllowed=dialog
DefaultDirName={autopf64}\TaylorLab
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\DoCombine.exe
SourceDir=M:\PracticeApps\WindowDesktopApps\PDFMerger\DoCombine\staging
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "*"; DestDir: "{app}\bin\"; Permissions: users-modify
Source: "ShellExt\*"; DestDir: "{app}\bin\ShellExt\"; Permissions: users-modify

[Icons]
Name: "{group}\DoCombine"; Filename: "{app}\bin\DoCombine.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall DoCombine"; Filename: "{uninstallexe}"

[Tasks]
Name: installcontextmenu; Description: "Add DoCombine to the context menu for PDF files (Can be done later in the app)"; GroupDescription: "Additional feature:";

[Registry]
; check installcontextmenu for each of these keys
Root: HKCU64; Subkey: "Software\Classes\CLSID\{{73b668a5-0434-4983-bb8a-8fab7c728e64}"; Flags: uninsdeletekey; Check: IsTaskSelected('installcontextmenu')
Root: HKCU64; Subkey: "Software\Classes\CLSID\{{73b668a5-0434-4983-bb8a-8fab7c728e64}\InprocServer32"; ValueName: ""; ValueType: string; ValueData: "{app}\bin\ShellExt\DoCombineShortcutMenu.dll"; Flags: uninsdeletekey; Check: IsTaskSelected('installcontextmenu')
Root: HKCU64; Subkey: "Software\Classes\CLSID\{{73b668a5-0434-4983-bb8a-8fab7c728e64}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"; Flags: uninsdeletekey; Check: IsTaskSelected('installcontextmenu')
Root: HKCU64; Subkey: "Software\Classes\SystemFileAssociations\.pdf\ShellEx\ContextMenuHandlers\DoCombineExt"; ValueType: string; ValueData: "{{73b668a5-0434-4983-bb8a-8fab7c728e64}"; Flags: uninsdeletekey; Check: IsTaskSelected('installcontextmenu')