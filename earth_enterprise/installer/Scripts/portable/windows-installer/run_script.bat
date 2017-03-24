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

@echo off

goto :script_documentation
This is the static script which will be copied manually to pulse runing machine.

It will run this script from script's base directory.

Expects atleast one argument
argument 1: The script (with its complete unix path) which is to be run
Rest of the arguments if any will be passed to this script.

usage:
      c:\pulse_running_dir\run_script.bat root@fusionqahost:%earth_enterprise_root%installer/Scripts/portable/windows-installer/create_windows_installer.bat py2exe
:script_documentation

cd %~dp0

REM Create a temporary directory tmp
set TMP_DIR=tmp-%RANDOM%
rmdir /q /s %TMP_DIR%
mkdir %TMP_DIR%
REM copy the batch file
C:\Googlestuff\Putty\pscp -i c:\private.ppk %1 %TMP_DIR%

REM get the batch file name
for /F "tokens=*" %%j in ('dir /b %TMP_DIR%') do set script_to_run=%%j

REM get all but first param as the first param is the remote location of this batch script itself
set params=
shift
:WHILE
IF [%1]==[] GOTO END_WHILE
set params=%params% %1
shift
GOTO WHILE
:END_WHILE

cd %TMP_DIR%
CALL "%script_to_run%"%params%
cd ..
rmdir /q /s %TMP_DIR%

