#!/bin/bash
#
# Usage: check_version.sh GMIC_PATH [gmic|CImg|check]
#
if [ "$2" = gmic ]; then
    grep "define gmic_version " $1/gmic.h | cut -d' ' -f3
fi
if [ "$2" = CImg ]; then
    grep "define cimg_version " $1/CImg.h | cut -d' ' -f3
fi
if [ "$2" = check ]; then
    CIMG_VERSION=$(grep "define cimg_version " $1/CImg.h | cut -d' ' -f3)
    GMIC_VERSION=$(grep "define gmic_version " $1/gmic.h | cut -d' ' -f3)
    if [ "${CIMG_VERSION}" = "${GMIC_VERSION}" ]; then
	exit 0
    else
	exit 1
    fi
fi
