REM
REM Copyright 2017 Google Inc.
REM
REM Licensed under the Apache License, Version 2.0 (the "License");
REM you may not use this file except in compliance with the License.
REM You may obtain a copy of the License at
REM
REM      http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing, software
REM distributed under the License is distributed on an "AS IS" BASIS,
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM See the License for the specific language governing permissions and
REM limitations under the License.
REM

@Echo off
goto :script_documentation
This DOS batch file initiates the process of windows portable server creation.
Always this batch file has to be copied to the windows machine where the pulse agent will be located. and it does the following steps:
  1. It copies all the required files for installer creation to current installer creation directory.
  2. It will call the required batch file to create the intaller.
  3. It will copy the installer executable back to server [fusionbuilder: 10.1.1.10 on private network]
  4. Delete all the copied files after completing above tasks.

Arguments:
  1. py2exe -> (convert python files to executable)
     Anything else or none, keep python files as it is.
:script_documentation

setLocal EnableDelayedExpansion

REM Step[1] Get the arguments and initailize all the required veriables.
REM user@10.1.1.10:/usr/local/google/pulse2/data/agents/master/work/02Fusion_Trunk_Installer/googleclient/geo/earth_enterprise/
set this_script_dir=%~dp0
set earth_enterprise_root=%1
shift
set pscp_path=C:\Googlestuff\Putty\pscp
set dest_path=%this_script_dir%gold\

REM Step[1.1] copy the installer list file from pulse client
%pscp_path% -i c:\private.ppk %earth_enterprise_root%legacy_installer/Scripts/portable/windows-installer/windows_installer_filelist.txt "%this_script_dir%windows_installer_filelist.txt"

REM Step[2] create the directory structure for copying installer files.
cd "%this_script_dir%"
if exist "%dest_path%" rmdir /s /q "%dest_path%"
mkdir "%dest_path%"

REM step[3] pscp files in filelist
for /f "eol=# tokens=1,2 delims= " %%i in (windows_installer_filelist.txt) do (
  set file_path=%%i
  set curr_path=%%j
  %pscp_path% -r -i c:\private.ppk %earth_enterprise_root%!file_path! "!dest_path!!curr_path!"
)

set user=%1
shift

REM step[4] Run the create installer batch file to create executable
cd "%dest_path%"
echo calling build_installer batch file to create installer...
CALL ..\build_installer.bat %1
echo created portable installer

REM [5]Rename the defualt installer with GEE naming convention.
echo renaming default installer with GEE naming convention...
For /F "tokens=2,3,4 delims=/ " %%A in ('Date /t') do (
  Set Month=%%A
  Set Day=%%B
  Set Year=%%C
)
set suffix_date=%Year%%Month%%Day%

if "%1" == "py2exe" (
  REM TODO: get version number from version.txt.
  set installer_name=GEEPortableWindowsInstaller-5.1.3-%suffix_date%-setup.exe
  set setupfile=C:\pulse_create_win_installer\installshield_project\Product Configuration 1\Release 1\DiskImages\DISK1\setup.exe
)

REM Step[6] copy the installer to fusion distribution directory
echo copying the portable installer to fusion distribution directory...
%pscp_path% -i c:\private.ppk "%setupfile%" %user%@10.1.1.10:/usr/local/google/gepackager/dist/%installer_name%

