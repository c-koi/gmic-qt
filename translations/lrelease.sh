#!/usr/bin/env bash
function usage()
{
  cat <<EOF
Usage:
       `basename $0` file.ts

   Call lrelease or lrelease-qt5 on file, depending on which command exists.

EOF
  exit 0
}

function die()
{
  local message="$@"
  >&2 echo "$message"
  exit 1
}

function exists()
{
  type "$1" >& /dev/null
}

in="$1"
[[ -z "$in" ]] && usage
[[ ! -e "$in" ]] && die "File not found: $in"
[[ ! $in =~ .*\.ts$  ]] && die "Not a ts file: $in"

if exists lrelease-qt5 ; then
  exec lrelease-qt5 -compress "$in"
elif exists lrelease ; then
  exec lrelease -compress "$in"
else
  die "No lrelease(-qt5) command available."
fi
