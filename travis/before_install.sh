#!/bin/bash

export PATH=$PATH:$QT_MACOS/bin
if [ "${TRAVIS_BRANCH}" == "release" ]; then
  export BRANCH="release"
else
  export BRANCH="unstable"
fi
LC_REPO_SLUG=$(echo "$TRAVIS_REPO_SLUG" | tr '[:upper:]' '[:lower:]')
LC_REPO_SLUG="${LC_REPO_SLUG}-${TRAVIS_BRANCH}"
export BRANCH_BUCKET="openkj-installers" 
export INSTALLERFN="OpenKJ-${OKJVER}-${BRANCH}-osx-installer.pkg"

chmod 755 ./travis/install.sh
chmod 755 ./travis/build.sh
