#!/bin/bash

set -e

# do not build mac for PR
if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
  exit 0
fi

BundlePath=$PWD/OpenKJ/OpenKJ.app

$HOME/Qt/5.12.6/clang_64/bin/qmake

make -j8

$HOME/Qt/5.12.6/clang_64/bin/macdeployqt ${BundlePath}
echo "Removing unneeded and non-appstore compliant plugins"
rm -f ${BundlePath}/Contents/PlugIns/sqldrivers/libqsqlmysql.dylib
rm -f ${BundlePath}/Contents/PlugIns/sqldrivers/libqsqlodbc.dylib
rm -f ${BundlePath}/Contents/PlugIns/sqldrivers/libqsqlpsql.dylib
echo "Copying GStreamer framework to package dir"
cp -pR /Library/Frameworks/GStreamer.framework.deploy ${BundlePath}/Contents/Frameworks/GStreamer.framework
echo "Fixing directory structure in the GStreamer framework"
rm -f ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current
cd ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions
ls -l
ln -s 1.0 Current
cd -
echo "Modifying linker path info for GStreamer library to app bundle pathing"
osxrelocator ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current/lib /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r &>/dev/null
osxrelocator ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r &>/dev/null
osxrelocator ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current/bin /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r &>/dev/null
osxrelocator ${BundlePath}/Contents/MacOS /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r &>/dev/null

echo "Signing code"
codesign -s "Application: Isaac Lightburn (47W8CPBS5A)" -vvvv --deep --timestamp=none ${BundlePath}
echo "Creating installer"
pwd
ls -l
ls -l /Users/travis/build/OpenKJ/OpenKJ/OpenKJ/OpenKJ.app
cp travis/dmgbkg.png ~/
ls -l travis/openkjdmg.json
#echo "Running appdmg travis/openkjdmg.json ${INSTALLERFN}"
#appdmg travis/openkjdmg.json ${INSTALLERFN}
mkdir okjimage
mv OpenKJ/OpenKJ.app okjimage/
echo "Running create-dmg to build installer"
bash ./create-dmg/create-dmg \
--volname "OpenKJ Installer" \
--volicon "OpenKJ/Icons/OpenKJ.icns" \
--background "travis/dmgbkg.png" \
--window-pos 200 120 \
--window-size 512 340 \
--icon-size 80 \
--icon "OpenKJ.app" 138 225 \
--hide-extension "OpenKJ.app" \
--app-drop-link 378 225 \
${INSTALLERFN} \
okjimage/

echo "Signing installer"
codesign -s "Application: Isaac Lightburn (47W8CPBS5A)" -vvvv --timestamp=none ${INSTALLERFN}

mkdir deploy
mkdir deploy/macos
mkdir deploy/macos/${BRANCH}
mv ${INSTALLERFN} deploy/macos/${BRANCH}/
