REM
:: Copyright 2017 Google Inc.
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::      http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.
::

@echo off
goto :script_documentation
This DOS batch file creates the directory hierarchy to be used for Windows installer. It does the following:
  1. Copies file manifest and then copies all files listed in manifest from master machine.
  2. Copies files and subdirectory to tmp_installshield_dir.
  3. Copies installshield project to installshield_project_dir.
  4. Builds the installler (setup.exe) using installshield.
:script_documentation

REM [1]Get required directory path (gold)
set curr_dir=%CD%
cd ..
set parent_dir=%CD%
cd ..
set parent_parent_dir=%CD%
set python_dir=c:\portable_gee\python265
set iscmdbld_dir=C:\Program Files (x86)\InstallShield\2011\System
set tmp_installshield_dir=%parent_parent_dir%\tmp_installshield_files
set installshield_project_dir=C:\pulse_create_win_installer\installshield_project
if "%1"=="py2exe" (
  cd "%curr_dir%"
  "%python_dir%\python.exe" py2exe_setup.py py2exe 
  REM Create portable.cfg file
  "%python_dir%\python.exe" creat_portable_cfg.py
  del *.py *.pyc
  for /F "tokens=*" %%j in ('dir /b dist') do move dist\%%j .
  rmdir /q /s dist
  rmdir /q /s build
  rmdir /q /s "%tmp_installshield_dir%"
  xcopy /e /i /q /s /y "%curr_dir%" "%tmp_installshield_dir%"
  cd %tmp_installshield_dir%
  move "ext_service_python" "ext_service.py"
  move "ge_base_search_python" "ge_base_search.py"
  move "search_service_geplaces_python" "search_service_geplaces.py"
  REM Generate pyc files for the above three files
  "%python_dir%\python.exe" -mcompileall .
  rmdir /q /s "%installshield_project_dir%"
  mkdir "%installshield_project_dir%"
  cd "%parent_dir%"
  move "GEEPortableISP.ism" "%installshield_project_dir%\GEEPortableISP.ism"
  REM Build installshield project file
  "%iscmdbld_dir%\IsCmdBld.exe" -p "%installshield_project_dir%\GEEPortableISP.ism" -r "Release 1" -a "Product Configuration 1"
)
