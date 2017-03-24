REM [0]
:: #
:: # Copyright 2017 Google Inc.
:: #
:: # Licensed under the Apache License, Version 2.0 (the "License");
:: # you may not use this file except in compliance with the License.
:: # You may obtain a copy of the License at
:: #
:: #      http://www.apache.org/licenses/LICENSE-2.0
:: #
:: # Unless required by applicable law or agreed to in writing, software
:: # distributed under the License is distributed on an "AS IS" BASIS,
:: # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: # See the License for the specific language governing permissions and
:: # limitations under the License.
:: #
:: #

@Echo off
SETLOCAL ENABLEDELAYEDEXPANSION

REM [1] Convert from mangled names to actual names using move.
if exist "%CD%.new" (
  rmdir /s /q "%CD%.new"
)

REM [1.1] Convert from 8 character encoded names to actual name.
echo Extracting GEE Portable Server Package to temp directory...
set dst_dir="%CD%.new"
mkdir %dst_dir%
for /F "tokens=*" %%i in ('dir /b') do (
  set orig=%%i
  if not "!orig!"=="%0" (
    move "!orig!" %dst_dir%
  )
)
CD /D %dst_dir%

REM [2] create the directory hirerchy from files where comma represent directory.
REM e.g convert from filename ./a,b,c to ./a/b/c
echo Preparing for Installation. Please wait...
for /F "tokens=*" %%i in ('dir /b "%CD%"') do (
  set orig=%%i
  set new=!orig:,=\!
  set complete_orig="%CD%\!orig!"
  set complete_new="%CD%\!new!"
  if not !complete_new!==!complete_orig! (
    for %%G in (!complete_new!) do (
      set dir_name="%%~dpG"
      if not exist !dir_name! (
        mkdir !dir_name!
      )
    )
    move !complete_orig! !complete_new!
  )
)


REM [2.2] This module will perform following tasks:
:: Check if portable.exe is already running and if the process is running then:
:: If user selects YES then it will stop the process and continues with  reinstallation
:: If user selects NO then it will terminate the installation.

if exist .\portable.exe (
  set status=0
  call:check_status_function status
  if !status!==1 (
    if "%GEE_PORTBALE_SILENT_INSTALLER_HOME%"=="" (
      call ".\portable.exe" get_yes_no "Installation Failed!" "Portable Server instance is already running on your machine.Click Yes,If you want to stop the server and continue with installation. Click No to exit installation." && (
        taskkill /F /IM portable.exe
        set status=0
      )
      REM To check when user pressed No option in the above dialog box
      call:check_status_function
      if !status!==1 (
        echo exiting installation ......
        cd ..
        rmdir /s /q %dst_dir%
        exit 3
      )
    ) else (
      echo Portable Server is already running!
      echo Please Stop the server and uninstall existing portable server setup before installation.
      rmdir /s /q %dst_dir%
      exit 3
    )
  )
)


REM [3] Get the Destination server and data directory via fileoutput of installer.py
if exist .\gepython\python.exe (
  .\gepython\python.exe get_user_dir_choice.py
) else (
  .\portable.exe get_user_dir_choice
)

REM [4] Call the setvar batchfile to set variables.
call geesetvar.bat
del /Q geesetvar.bat

REM [5] Exit If variable is not set.
if "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"=="" (
  echo Invalid server installation directory.Installation Failed!!!
  echo Please try again.
  pause
  exit 2
)

if not "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"=="%GEE_PORTABLE_SILENT_INSTALLER_DATA%" (
  echo @echo off > move_glb.bat
  for /F "tokens=*" %%i in ('dir /b *.glb') do (
    echo move "%%i" "%GEE_PORTABLE_SILENT_INSTALLER_DATA%" >> move_glb.bat
  )
  for /F "tokens=*" %%i in ('dir /b *.glm') do (
    echo move "%%i" "%GEE_PORTABLE_SILENT_INSTALLER_DATA%" >> move_glb.bat
  )
)

if not "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"=="%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%" (
  echo @echo off > move_search.bat
  echo move "ge_base_search_python" "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%\ge_base_search.py" >> move_search.bat
  echo move "search_service_geplaces_python" "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%\search_service_geplaces.py" >> move_search.bat
  echo move "places_200.txt" "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%\geplaces" >> move_search.bat
)

if not "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"=="%GEE_PORTABLE_SILENT_INSTALLER_EXT%" (
  echo @echo off > move_ext.bat
  echo move "ext_service_python" "%GEE_PORTABLE_SILENT_INSTALLER_EXT%\ext_service.py" >> move_ext.bat
)

REM [6] Move to server directory as per user input.
if exist "%GEE_PORTABLE_SILENT_INSTALLER_HOME%" (
  REM [6a] If the server directory exists move each file, so that it is upgrade
  for /F "tokens=*" %%i in ('dir /b /s /a-d "%CD%"') do (
    set orig="%%i"
    set dst_name=!orig:%CD%=%GEE_PORTABLE_SILENT_INSTALLER_HOME%!
    for %%K in (!dst_name!) do (
      set dir_name="%%~dpK"
      if not exist !dir_name! (
        mkdir !dir_name!
      )
    )
    move !orig! !dst_name!
  )
) else (
  REM [6b] If the server directory doesn't exist move top level dir_name
  cd ..
  for %%K in (%GEE_PORTABLE_SILENT_INSTALLER_HOME%) do (
    set dir_name="%%~dpK"
    if not exist !dir_name! (
      mkdir !dir_name!
    )
  )
  move %dst_dir% "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"
)

rmdir /s /q %dst_dir%

REM [7] Create data directory and copy all globes to new directory
if not exist "%GEE_PORTABLE_SILENT_INSTALLER_DATA%" (
  mkdir "%GEE_PORTABLE_SILENT_INSTALLER_DATA%"
)
if not exist "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%" (
  mkdir "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%"
)
if not exist "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%\geplaces" (
  mkdir "%GEE_PORTABLE_SILENT_INSTALLER_SEARCH%\geplaces"
)
if not exist "%GEE_PORTABLE_SILENT_INSTALLER_EXT%" (
  mkdir "%GEE_PORTABLE_SILENT_INSTALLER_EXT%"
)
if not "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"=="%GEE_PORTABLE_SILENT_INSTALLER_DATA%" (
  CD /D "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"
  call "%GEE_PORTABLE_SILENT_INSTALLER_HOME%\move_glb.bat"
  del /Q "%GEE_PORTABLE_SILENT_INSTALLER_HOME%\move_glb.bat"
  call "%GEE_PORTABLE_SILENT_INSTALLER_HOME%\move_search.bat"
  del /Q "%GEE_PORTABLE_SILENT_INSTALLER_HOME%\move_search.bat"
  call "%GEE_PORTABLE_SILENT_INSTALLER_HOME%\move_ext.bat"
  del /Q "%GEE_PORTABLE_SILENT_INSTALLER_HOME%\move_ext.bat"
)

REM [8] Create shortcuts.
echo creating shortcuts...
if exist .\gepython\python.exe (
  set cmd_part=.\gepython\python.exe creatshortcut.py
) else (
  set cmd_part=.\portable.exe creatshortcut
)
if not "%GEE_PORTABLE_SILENT_INSTALLER_DSHORTCUT%"=="" (
  if not "%GEE_PORTABLE_SILENT_INSTALLER_SSHORTCUT%"=="" (
    CD /D "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"
    !cmd_part! -D "%GEE_PORTABLE_SILENT_INSTALLER_DSHORTCUT%" -S "%GEE_PORTABLE_SILENT_INSTALLER_SSHORTCUT%"
  )
) else (
  CD /D "%GEE_PORTABLE_SILENT_INSTALLER_HOME%"
  !cmd_part!
)
echo "Installation Done Successfully"


::-------------------------------------------------------------------
::---- Function to get the check if portable.exe process is running
::-------------------------------------------------------------------
:check_status_function
  tasklist /FI "IMAGENAME eq portable.exe" /FI "STATUS eq RUNNING" /FO CSV > %temp%\status.log
  for /F "tokens=*" %%i in ('findstr "portable.exe" %temp%\status.log') do (
    set result=%%i
    if not "!result!"=="" (
      set %~1=1
    )
  )
  del /s /q %temp%\status.log
