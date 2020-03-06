#!/bin/bash

# do not build mac for PR
if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
  exit 0
fi

echo "PWD: $PWD"
echo "BRANCH_BUCKET=${BRANCH_BUCKET}"

echo "Setting up signing environment"
security create-keychain -p $keychainPass build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p $keychainPass build.keychain
security set-keychain-settings -t 3600 -u build.keychain


wget -c --no-check-certificate -nv -Ocscrt.zip https://storage.googleapis.com/okj-installer-deps/devidapplication.zip

unzip -P$cscrtPass cscrt.zip

security import devidapplication.p12 -k build.keychain -P $p12Pass -A 
security set-key-partition-list -S apple-tool:,apple: -s -k $keychainPass build.keychain

echo "Installing osxrelocator"
pip2 install osxrelocator

#echo "Installing appdmg"
#npm install -g appdmg

echo "Grabbing create-dmg"
wget -c --no-check-certificate -nv -Ocreate-dmg.zip https://storage.googleapis.com/okj-installer-deps/create-dmg-1.0.0.5.zip
unzip create-dmg.zip
mv create-dmg-1.0.0.5 create-dmg

if [ -d "Qt" ]; then
  echo "Cached copy of Qt already exists, skipping install"
else
  #install gstreamer#install Qt
  echo "Downloading Qt"
  wget -c --no-check-certificate -nv -Oqt.tar.bz2 https://storage.googleapis.com/okj-installer-deps/qt-5.12.6.tbz2
  echo "Extracting Qt"
  bunzip2 -v qt.tar.bz2
  echo "Untarring Qt"
  tar -xf qt.tar
  echo "Moving Qt to proper location"
  mv Qt $HOME/Qt
fi

if [ -d "/Library/Frameworks/GStreamer.framework" ]; then
  echo "Cached copy of gstreamer already exists, skipping installation"
else
  echo "gstreamer install"
  echo "Downloading gstreamer runtime package"
#  wget -c --no-check-certificate -nv -Ogstreamer.pkg https://storage.googleapis.com/okj-installer-deps/gstreamer-1.0-1.11.2-x86_64.pkg 
  wget -c --no-check-certificate -nv -Ogstreamer.pkg https://storage.googleapis.com/okj-installer-deps/gstreamer-1.0-1.16.2-x86_64.pkg
  echo "Downloading gstreamer devel package"
#  wget -c --no-check-certificate -nv -Ogstreamer-dev.pkg https://storage.googleapis.com/okj-installer-deps/gstreamer-1.0-devel-1.11.2-x86_64.pkg
  wget -c --no-check-certificate -nv -Ogstreamer-dev.pkg https://storage.googleapis.com/okj-installer-deps/gstreamer-1.0-devel-1.16.2-x86_64.pkg 
  echo "Installing gstreamer runtime package"
  sudo installer -package gstreamer.pkg -target /;
  echo "Making a deployment copy of the runtime"
  sudo cp -R /Library/Frameworks/GStreamer.framework /Library/Frameworks/GStreamer.framework.deploy
  echo "Installing gstreamer devel package"
  sudo installer -package gstreamer-dev.pkg -target /;
  sudo ln -s /Users/travis /Users/lightburnisaac
  echo "gstreamer install done"
fi


