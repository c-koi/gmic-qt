#!/bin/bash
#
# Usage: check_version.sh GMIC_PATH [gmic|CImg|check]
#
if [ $# != 2 ]; then
    echo
    echo "  Usage:"
    echo "        check_version.sh GMIC_PATH [gmic|CImg|check]"
    echo
fi
if [ "$2" = gmic ]; then
    grep "define gmic_version " $1/gmic.h | cut -d' ' -f3
fi
if [ "$2" = CImg ]; then
    grep "define cimg_version " $1/CImg.h | cut -d' ' -f3
fi
if [ "$2" = check ]; then
    CIMG_VERSION=$(grep "define cimg_version " $1/CImg.h | cut -d' ' -f3)
    GMIC_VERSION=$(grep "define gmic_version " $1/gmic.h | cut -d' ' -f3)
    test "${CIMG_VERSION}" = "${GMIC_VERSION}"
fi
