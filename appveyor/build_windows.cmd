echo on

SET project_dir="%cd%"

echo Building OpenKJ branch %APPVEYOR_REPO_BRANCH%

echo Set up environment...
set PATH=%QT%\bin\;C:\Qt\Tools\QtCreator\bin\;C:\Qt\QtIFW2.0.1\bin\;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %PLATFORM%

mkdir "%project_dir%\cscrt
7z e "%project_dir%\appveyor\cscrt.7z" -p"%cscrt_pass%" -o"%project_dir%\cscrt"

echo Building OpenKJ...
qmake CONFIG-=debug CONFIG+=release
nmake

echo Packaging...
cd %project_dir%\build\windows\msvc\%LONGARCH%\release\
dir
windeployqt OpenKJ\release\OpenKJ.exe
echo Signing OpenKJ binary
signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /f "%project_dir%\cscrt\cscrt.pfx" /p "%pfx_pass%" OpenKJ\release\Openkj.exe

echo Copying project files for archival...
copy "%project_dir%\README.md" "OpenKJ\release\README.md"
copy "%project_dir%\LICENSE" "OpenKJ\release\LICENSE.txt"

mkdir "%project_dir%\output"

echo Copying files for installer...
robocopy OpenKJ\release\ "%project_dir%\output" /E /np
del "%project_dir%\output\*.obj"
del "%project_dir%\output\*.cpp"
del "%project_dir%\output\*.h"


echo Pulling gstreamer deps for installer...
copy c:\gstreamer\1.0\%LONGARCH%\bin\*.dll "%project_dir%\output\"
mkdir "%project_dir%\output\lib"
mkdir "%project_dir%\output\lib\gstreamer-1.0"
mkdir "%project_dir%\output\lib\gstreamer-1.0\validate"
copy c:\gstreamer\1.0\%LONGARCH%\lib\gstreamer-1.0\*.dll "%project_dir%\output\lib\gstreamer-1.0\"
copy c:\gstreamer\1.0\%LONGARCH%\lib\gstreamer-1.0\validate\*.dll "%project_dir%\output\lib\gstreamer-1.0\validate\"

echo Creating installer...
cd %project_dir%\installer\windows\%LONGARCH%\
dir
rem binarycreator.exe --offline-only -c config\config.xml -p packages OpenKJ-%OKJVERSION%-windows-%LONGARCH%-installer.exe
"C:\Program Files (x86)\Inno Setup 5\iscc.exe" "%project_dir%\appveyor\openkj_%LONGARCH%.iss" /O"%project_dir%/"
move "%project_dir%\OpenKJ.exe" "%project_dir%\OpenKJ-%OKJVERSION%-%BITS%-setup.exe"
echo Signing installer...
signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /f "%project_dir%\cscrt\cscrt.pfx" /p "%pfx_pass%" "%project_dir%\OpenKJ-%OKJVERSION%-%BITS%-setup.exe
