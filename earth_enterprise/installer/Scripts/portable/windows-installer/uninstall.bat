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

REM This file will be automatically edited by installer.py file and it will add the step[1] content to this file.

setLocal EnableDelayedExpansion

REM [2]Prompt and exit,If Server is running.
cd %tools_dir%
if exist "%tools_dir%\gepython\python.exe" (
  set check_server_status_cmd="%tools_dir%\gepython\python.exe" "%tools_dir%\check_server_running.py"
) else (
  set check_server_status_cmd="%tools_dir%\portable.exe" check_server_running
)
call !check_server_status_cmd! || (
  goto:EOF
)

REM [3]Delete Destop and showmenu shortcuts.
cd "%shortcut_dir%"
del StartServer.lnk RemoteServer.lnk
cd "%menu_dir%"
rmdir /q /s gee_portable

REM [4]Check for silent uninstallation of data directory,If not get confirmation for deleting data directory.
cd \
if "%GEE_PORTABLE_SILENT_UNINSTALLER_DELETE_DATA%"=="" (
  if exist "%tools_dir%\gepython\python.exe" (
    set cmd_part="%tools_dir%\gepython\python.exe" "%tools_dir%\get_yes_no.py"
  ) else (
    set cmd_part="%tools_dir%\portable.exe" get_yes_no
  )
  call !cmd_part! "Uninstall Portable Server" "Do you want to delete data directory with all globes in it?" && rmdir /q /s "%data_dir%"
) else (
  if /I "%GEE_PORTABLE_SILENT_UNINSTALLER_DELETE_DATA%"=="Y" (
    rmdir /q /s "%data_dir%"
  )
)
REM [5]Delete Tools directory
rmdir /q /s "%tools_dir%"
