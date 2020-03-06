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
osxrelocator ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current/lib /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r 
osxrelocator ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current/libexec /Library/Frameworks/GStreamer.framework/ @executable_path/../../../../../GStreamer.framework/ -r 
osxrelocator ${BundlePath}/Contents/Frameworks/GStreamer.framework/Versions/Current/bin /Library/Frameworks/GStreamer.framework/ @executable_path/../../../../GStreamer.framework/ -r 
osxrelocator ${BundlePath}/Contents/MacOS /Library/Frameworks/GStreamer.framework/ @executable_path/../Frameworks/GStreamer.framework/ -r &>/dev/null

echo "Signing code"
codesign -s "Developer ID Application: Isaac Lightburn (47W8CPBS5A)" -vvvv --deep --timestamp=none ${BundlePath}

echo "Creating installer pkg"
mv ${BundlePath} /Applications/OpenKJ.app
productbuild --identifier org.openkj.openkj --version ${OKJVER} --component /Applications/OpenKJ.app OpenKJ.pkg

echo "Signing installer"
productsign --sign "Developer ID Installer: Isaac Lightburn (47W8CPBS5A)" OpenKJ.pkg ${INSTALLERFN}

echo "Deploying installer"
mkdir deploy
mkdir deploy/macos
mkdir deploy/macos/${BRANCH}
mv ${INSTALLERFN} deploy/macos/${BRANCH}/
