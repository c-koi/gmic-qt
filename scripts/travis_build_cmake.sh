#!/bin/bash
set -ev

if [ "${TRAVIS_BRANCH}" = devel ]; then
    BUILD_TYPE=Debug
else
    BUILD_TYPE=Release
fi

cmake --version

GMIC_PATH=$(pwd)/gmic-clone/src

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DGMIC_PATH=${GMIC_PATH} -DGMIC_QT_HOST=${GMIC_HOST} ..
make VERBOSE=1
