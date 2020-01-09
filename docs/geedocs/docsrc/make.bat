@ECHO OFF

REM Command file for Sphinx documentation

if "%SPHINXBUILD%" == "" (
	set SPHINXBUILD=sphinx-build
)

set SPHINXOPTS=-W
set SOURCEDIR=.
set BUILDDIR=build
set SPHINXPROJ=GoogleEarthEnterpriseDocumentation

set SPHINX2GITHUB=python sphinxtogithub.py
set GETOPENGEEVER=python ..\..\..\earth_enterprise\src\scons\getversion.py

%GETOPENGEEVER% > temp.txt
set /p OPENGEEVER=< temp.txt
del temp.txt

if "%1" == "" goto help

%SPHINXBUILD% 2> nul
if errorlevel 9009 (
	echo.
	echo.The 'sphinx-build' command was not found. Make sure you have Sphinx
	echo.installed, then set the SPHINXBUILD environment variable to point
	echo.to the full path of the 'sphinx-build' executable. Alternatively you
	echo.may add the Sphinx directory to PATH.
	echo.
	echo.If you don't have Sphinx installed, grab it from
	echo.http://sphinx-doc.org/
	exit /b 1
)

echo.Deleting %BUILDDIR% directory
rmdir /S /Q %BUILDDIR%
echo.Generating html
%SPHINXBUILD% -M %1 %SOURCEDIR% %BUILDDIR% %SPHINXOPTS%
echo.Removing _ in all directories under html
%SPHINX2GITHUB% "%BUILDDIR%\html"
echo.Deleting %OPENGEEVER% directory
rmdir /S /Q ..\%OPENGEEVER%
echo.Creating %OPENGEEVER% directory
mkdir ..\%OPENGEEVER%
echo.Copying %BUILDDIR%\html to ..\%OPENGEEVER% directory
xcopy /E /K .\%BUILDDIR%\html\* ..\%OPENGEEVER%

goto end

:help
%SPHINXBUILD% -M help %SOURCEDIR% %BUILDDIR% %SPHINXOPTS%

:end
