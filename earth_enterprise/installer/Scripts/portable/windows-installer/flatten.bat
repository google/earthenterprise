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

REM Flattens a directory hierarchy to single hierarchy and moves to the destination directory
SETLOCAL ENABLEDELAYEDEXPANSION

REM [input argument] Source directory (say X). The files in it will be renamed so that the REM sub directory structure is embedded in filename.
REM For example X/a/b/c will be renamed to a,b,c.
set inp_dir=%1

REM [input argument] Destination directory to store flat files (say Y).
REM For example X/a/b/c will be stored as Y/a,b,c.
set dst_dir=%2

if exist "!dst_dir!" rmdir /s /q "!dst_dir!"
mkdir "!dst_dir!"

set unquoted=!inp_dir:~1,-1!
set dst=!dst_dir:~1,-1!
set orig=
for /F "tokens=*" %%i in ('dir /b /s /a:-d %inp_dir%') do (
  set orig=%%i
  for /F "tokens=*" %%j in ('echo "%%orig:%unquoted%=%%"') do (
    set relative=%%j
  )
  set k=!relative:~2,-1!
  set l=!k:\=,!
  move "!orig!" "!dst!\!l!"
)
