#!/bin/bash
set -o errexit
function usage()
{
  echo "  Usage:"
  echo
  echo "        check_version.sh GMIC_PATH [gmic|CImg|stdlib]"
  echo
  exit 0
}

function die()
{
  local message="$1"
  >&2 echo "Error: $*"
  exit 1
}

(( $# == 0 )) && usage
[[ -d "$1" ]] || die "$1 is not an existing directory"

# @param folder
function gmic_version()
{
  local folder="$1"
  [[ -e "${folder}/gmic.h" ]] || die "File not found: ${folder}/gmic.h"
  local version=$(grep -F "#define gmic_version " ${folder}/gmic.h)
  echo ${version//* }
}

# @param folder
function cimg_version()
{
  local folder="$1"
  [[ -e "${folder}/CImg.h" ]] || die "File not found: ${folder}/CImg.h"
  local version=$(grep -F "#define cimg_version " ${folder}/CImg.h)
  echo ${version//* }
}

# @param folder
function stdlib_version()
{
  local folder="$1"
  [[ -e "${folder}/gmic_stdlib_community.h" ]] || die "File not found: ${folder}/gmic_stdlib_community.h"
  local version=$(grep -E "File.*gmic_stdlib_community.h.*\(v." ${folder}/gmic_stdlib_community.h)
  version=${version#*v.}
  version=${version%)}
  version=${version//.}
  echo ${version}
}

if [[ "$2" == gmic ]]; then
  gmic_version "$1"
  exit 0
fi

if [[ "$2" == CImg ]]; then
  cimg_version "$1"
  exit 0
fi

if [[ "$2" == stdlib ]]; then
  stdlib_version "$1"
  exit 0
fi

echo "Checking G'MIC and CImg versions..."

GMIC_VERSION=$(gmic_version "$1")
CIMG_VERSION=$(cimg_version "$1")
STDLIB_VERSION=$(stdlib_version "$1")

echo "G'MIC version is .................... $GMIC_VERSION"
echo "gmic_stdlib_community.h version is .. $STDLIB_VERSION"
echo "CImg version is ..................... $CIMG_VERSION"

if [[ $GMIC_VERSION != $CIMG_VERSION ]]; then
  die "Version numbers of files 'gmic.h' (${GMIC_VERSION}) and 'CImg.h' (${CIMG_VERSION}) mismatch"
fi

if [[ $GMIC_VERSION != $STDLIB_VERSION ]]; then
  die "Version numbers of files 'gmic.h' (${GMIC_VERSION}) and 'gmic_stdlib_community.h' (${STDLIB_VERSION}) mismatch"
fi

exit 0
