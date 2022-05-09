#!/bin/bash
#
# Usage: check_version.sh GMIC_PATH [gmic|CImg|stdlib]
#
if [ $# != 2 ]; then
  echo
  echo "  Usage:"
  echo "        check_version.sh GMIC_PATH [gmic|CImg|stdlib]"
  echo
fi
if [ "$2" = gmic ]; then
  grep "define gmic_version " $1/gmic.h | cut -d' ' -f3
fi
if [ "$2" = CImg ]; then
  grep "define cimg_version " $1/CImg.h | cut -d' ' -f3
fi
if [ "$2" = stdlib ]; then
  VERSION=$(grep "File.*gmic_stdlib_community.h.*(v." $1/gmic_stdlib_community.h)
  VERSION=${VERSION#*v.}
  VERSION=${VERSION%)}
  VERSION=${VERSION//.}
  echo "$VERSION"
fi
