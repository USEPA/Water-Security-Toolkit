; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "TEVA-SPOT Toolkit"
#define MyAppPublisher "Sandia Corporation"
#define MyAppURL "http://software.sandia.gov/trac/spot"
#define MyAppCopyright "2007-2009 Sandia Corporation"
; Comment out the following for normal operation. Uncomment for local testing
;#define MyAppVersion "2.3"
;#define MySetupVersion "2.3"
;#define MyAppRevision "(r9283)"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{F7C2F737-3106-44F3-A507-1EC2A5AAC1D3}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion} {#MyAppRevision}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={code:GetDeSpacedName|{pf}\pyspot}
DefaultGroupName={#MyAppName}
InfoBeforeFile=RELEASE_NOTES.txt
OutputBaseFilename=teva-spot-{#MyAppVersion}
Compression=lzma
SolidCompression=true
SourceDir=.
VersionInfoVersion={#MySetupVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName}
VersionInfoTextVersion={#MyAppName} {#MySetupVersion} {#MyAppRevision}
VersionInfoCopyright={#MyAppCopyright}
AppCopyright={#MyAppCopyright}
RestartIfNeededByRun=true
PrivilegesRequired=none
ChangesEnvironment=true
AlwaysUsePersonalGroup=false
AllowNoIcons=true
UninstallDisplayName={#MyAppName} {#MyAppVersion} {#MyAppRevision}
;UserInfoPage=true
;DefaultUserInfoName=ignore
;DefaultUserInfoOrg=
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
OutputDir=../../output

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Files]
Source: ../tpl/pywst/util/wst_install; DestDir: {app}/dist
Source: ../../build/tevaspot_acro.zip; DestDir: {app}/dist
Source: ../../build/pywst_bin_win32.zip; DestDir: {app}/dist; Check: IsWin32; Flags: 32bit
Source: ../../build/pywst_py_win32.zip; DestDir: {app}/dist; Check: IsWin32; Flags: 32bit
Source: ../../build/pywst_bin_win64.zip; DestDir: {app}/dist; Check: IsWin64; Flags: 64bit
Source: ../../build/pywst_py_win64.zip; DestDir: {app}/dist; Check: IsWin64; Flags: 64bit

[Icons]
;Name: {group}\TEVA-SPOT; Filename: {app}\bin\teva-spot.cmd; WorkingDir: {app}; Comment: Opens a command prompt in the TEVA-SPOT directory with the appropriate paths
;
; NOTE: group names need to be quoted to properly handle spaces
;
Name: {group}\Run TEVA-SPOT Toolkit; Filename: {app}\bin\teva-spot.cmd; Comment: Launches a window ready to run TEVA SPOT Toolkit; Languages: 
Name: {group}\TEVA-SPOT User's Guide; Filename: {app}\doc\userguide.pdf; Comment: Open TEVA-SPOT User's Guide in AcroRead
Name: {group}\Getting Started; Filename: {app}\doc\gstarted.pdf
Name: {group}\Uninstall TEVA-SPOT; Filename: {uninstallexe}

[Run]
Filename: {code:GetPythonPath}python.exe; Parameters: {app}/dist/wst_install --offline --zip={app}/dist/pywst_py_win32.zip --zip={app}/dist/pywst_bin_win32.zip --zip={app}/dist/tevaspot_acro.zip {app}; WorkingDir: {app}\..; Flags: 32bit; Check: IsWin32; StatusMsg: Installing TEVA-SPOT Toolkit... (32-bit executables)
Filename: {code:GetPythonPath}python.exe; Parameters: {app}/dist/wst_install --offline --zip={app}/dist/pywst_py_win64.zip --zip={app}/dist/pywst_bin_win64.zip --zip={app}/dist/tevaspot_acro.zip {app}; WorkingDir: {app}\..; Flags: 64bit; Check: IsWin64; StatusMsg: Installing TEVA-SPOT Toolkit... (64-bit executables)

[Dirs]
Name: {app}
Name: {app}\dist

[Registry]

;[Components]
;Name: Core; Description: Basic core files needed for TEVA-SPOT; Flags: fixed; Types: custom compact full
;Name: Documentation; Description: Documentation for TEVA-SPOT; ExtraDiskSpaceRequired: 2; Types: custom compact full
;Name: Examples; Description: Example files for TEVA-SPOT; ExtraDiskSpaceRequired: 1; Types: custom full
;Name: Tests; Description: Testing data for TEVA-SPOT; ExtraDiskSpaceRequired: 151; Types: custom full

[Messages]
;UserInfoName=&User Name:
;UserInfoDesc=If necessary, please enter the proxy server information needed to access the internet. If no proxy is needed, leave blank.
;UserInfoOrg=Proxy IP/Hostname and Port:
FinishedLabel=Setup has finished installing [name] on your computer. The application may be launched by selecting the [name] icon from the Start menu.  This will open a console with the correct path and environment variables.
WelcomeLabel2=This will install [name/ver] on your computer.%n%nPython version 2.5/2.6/2.7 is required to run [name]. If you do not already have Python, please obtain and install it from %n    http://www.python.org%nprior to installing [name].%n%nIt is recommended that you close all other applications before continuing.

[UninstallDelete]
Name: {app}; Type: filesandordirs; Languages: 

[Code]
var
  PythonInstallChecked: Boolean;
  TempPathValue: String;
  PythonInstallPath: String;
  PythonExists: Boolean;
  Version: TWindowsVersion;

function GetDeSpacedName(Param: String): String;
begin
  StringChangeEx(Param, ' ', '%20', True);
  Result := Param;
end;

function IsWin32(): Boolean;
begin
  Result := True;
  if IsWin64() then begin
    Result := False;
  end;
end;

function PrepareToInstall(): String;
begin
  Result := '';
  if not PythonInstallChecked then begin
    PythonExists := False;
    if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.5') then
    begin
      PythonExists := True;
    end;
    if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.6') then
    begin
      PythonExists := True;
    end;
    if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.7') then
    begin
      PythonExists := True;
    end;
    if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.5') then
    begin
      PythonExists := True;
    end;
    if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.6') then
    begin
      PythonExists := True;
    end;
    if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.7') then
    begin
      PythonExists := True;
    end;
    PythonInstallChecked := True;
  end;
  if not PythonExists then begin
    Result := 'An acceptable version of Python was not found on your system. Installation requires Python 2.5 or greater, but has not been tested with Python 3.x yet. Please try this installer again after installing Python.';
  end;
end;

function GetPythonPath(Param: String): String;
begin
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.5') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.5\InstallPath',
        '', TempPathValue);
  end;
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.6') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.6\InstallPath',
        '', TempPathValue);
  end;
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.7') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\Python\PythonCore\2.7\InstallPath',
        '', TempPathValue);
  end;
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.5') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.5\InstallPath',
        '', TempPathValue);
  end;
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.6') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.6\InstallPath',
        '', TempPathValue);
  end;
  if RegKeyExists(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.7') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE, 'Software\WOW6432Node\Python\PythonCore\2.7\InstallPath',
        '', TempPathValue);
  end;
  PythonInstallPath := TempPathValue;
  Result := PythonInstallPath;
end;