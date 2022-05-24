#!/usr/bin/env bash
function usage()
{
  cat <<EOF
Usage:
       `basename $0` -o output.csv file.ts

EOF
  exit 0
}

output=/dev/stdout
while getopts o: opt ; do
  case $opt in
    o) output=$OPTARG ;;
    *) die "Command line parsing failed"
  esac
done

n=$(( OPTIND - 1  ))
while [[ $n -gt 0 ]]; do
  shift
  n=$((n - 1))
done

function die()
{
  local message="$@"
  >&2 echo "$message"
  exit 1
}

function requires()
{
  type "$1" >& /dev/null || die "Command '$1' not found."
}

in="$1"
[[ -z "$in" ]] && usage
[[ ! -e "$in" ]] && die "File not found: $in"
[[ ! $in =~ .*\.ts$  ]] && die "Not a ts file: $in"

# @param text
function html2ascii()
{
  local text="$1"
  text=${text//&amp;/&}
  text=${text//&lt;/<}
  text=${text//&gt;/>}
  text=${text//&quot;/\"}
  text=${text//&apos;/\'}
  echo -n "$text"
}

echo -n > $output

while read line; do
  comment=""
  if [[ "$line" =~ \ *\<message\> ]]; then
    read src
    read translation
    if [[ "$translation" =~ \<comment\>.*\</comment\> ]]; then
      comment="$translation"
      read translation
    fi
    read dummy # </message>
    src=$(echo "$src")
    src=${src:8}
    src=${src%</source>}
    src=$(html2ascii "$src")
    translation=$(echo "$translation")
    translation=${translation:13}
    translation=${translation%</translation>}
    translation=$(html2ascii "$translation")
    if [[ -z "$comment" ]]; then
      echo "$src , $translation" >> $output
    else
      comment=$(echo "$comment")
      comment=${comment:9}
      comment=${comment%</comment>}
      echo "$src , $translation , $comment" >> $output
    fi
  fi
done < "$in"
