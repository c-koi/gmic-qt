#!/bin/bash
set -ev

#config=debug
config=release

qmake --version

echo "Building standalone plugin"
qmake CONFIG+=${config} HOST=none GMIC_PATH=gmic-clone/src
make

echo "Building Gimp plugin"
qmake CONFIG+=${config} HOST=gimp GMIC_PATH=gmic-clone/src
make
make

echo "Building Krita plugin"
qmake CONFIG+=${config} HOST=krita GMIC_PATH=gmic-clone/src
make
make
