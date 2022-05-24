#!/usr/bin/env bash
function usage()
{
  cat <<EOF
Usage:
       `basename $0` -o output.ts file.csv

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

requires sed

input="$1"
[[ -z "$input" ]] && usage
[[ ! -e "$input" ]] && die "File not found: $input"
[[ ! $input =~ .*\.csv$  ]] && die "Not a csv file: $input"

if [[ $input =~ gmic_qt.*\.csv ]]; then
  language=${input%%.csv}
  language=${language##*_}
else
  language=${input%%.csv}
  language=${language%%_*}
fi

cat <<EOF > $output
<?xml version="1.0" encoding="utf-8"?>

<!DOCTYPE TS>
<TS version="2.1" language="${language}">
<context>
  <name>FilterTextTranslator</name>
EOF

# @param name
# @param text
function xml_tag()
{
  local name="$1"
  local text="$2"
  text=${text//&/&amp;}
  text=${text//\"/&quot;}
  text=${text//\'/&apos;}
  text=${text//</&lt;}
  text=${text//>/&gt;}
  text=${text# }
  echo "      <${name}>${text}</$name>"
}

# @param source
# @param translation
# @param comment (optionnal)
function message()
{
  local source="$1"
  local translation="$2"
  local comment="$3"
  echo -e "\n    <message>"
  xml_tag source "$source"
  [[ -n "$comment" ]] && xml_tag comment "$comment"
  xml_tag translation "$translation"
  echo "    </message>"
}

while IFS=$'\t' read -r -a columns ; do
  count=${#columns[@]}
  if (( count > 2 )); then
    i=2
    while (( i < count )); do
      message "${columns[0]}" "${columns[1]}" "${columns[$i]}"  >> $output
      i=$((i+1))
    done
  elif (( count == 2 )); then
    message "${columns[0]}" "${columns[1]}" >> $output
  fi
done < <(sed -e 's/ , /\t/g' "$input")

cat <<EOF >> $output

</context>
</TS>
EOF
