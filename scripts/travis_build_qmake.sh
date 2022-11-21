#!/bin/bash
set -ev

if [ "${TRAVIS_BRANCH}" = devel ]; then
    config=debug
else
    config=release
fi

qmake --version

echo "Building standalone plugin"
qmake CONFIG+=${config} HOST=none GMIC_PATH=gmic-clone/src
make

echo "Building Gimp plugin"
qmake CONFIG+=${config} HOST=gimp GMIC_PATH=gmic-clone/src
make
make
