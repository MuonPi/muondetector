#!/usr/bin/env bash
set -euo pipefail
shopt -s extglob

usage() {
    cat >&2 <<'EOF'
Usage: format.sh <reader_executable> [reader_arguments...]

Example:
  ./format.sh ./sds011_reader /dev/ttyUSB0 9600 1
  ./format.sh ./bme280_reader 1

The formatter accepts comma-separated or whitespace-separated *_reader output.
EOF
}

status() {
    printf '# %s\n' "$*" >&2
}

trim() {
    local value=$1
    value=${value%$'\r'}
    value=${value##+([[:space:]])}
    value=${value%%+([[:space:]])}
    printf '%s' "$value"
}

is_numeric() {
    [[ $1 =~ ^[-+]?(([0-9]+([.][0-9]*)?)|([.][0-9]+))([eE][-+]?[0-9]+)?$ ]]
}

split_line() {
    local line=$1
    fields=()

    if [[ $delimiter == csv || ( -z $delimiter && $line == *,* ) ]]; then
        local raw_fields=()
        IFS=',' read -r -a raw_fields <<<"$line"
        for field in "${raw_fields[@]}"; do
            fields+=("$(trim "$field")")
        done
        detected_delimiter=csv
    else
        read -r -a fields <<<"$line"
        detected_delimiter=space
    fi
}

looks_like_header() {
    local count=${#fields[@]}
    local uppercase_count=0
    local underscore_seen=0

    [[ $count -gt 0 ]] || return 1

    local field
    for field in "${fields[@]}"; do
        if [[ $field == *:* ]] || is_numeric "$field"; then
            return 1
        fi
        if [[ ! $field =~ ^[A-Za-z_][A-Za-z0-9_.\/%+-]*$ ]]; then
            return 1
        fi
        if [[ $field == *_* ]]; then
            underscore_seen=1
        fi
        if [[ $field =~ ^[A-Z0-9_/%+-]+$ ]]; then
            ((++uppercase_count))
        fi
    done

    [[ $underscore_seen -eq 1 || $uppercase_count -eq $count ]]
}

init_generated_header() {
    header=()
    local count=${#fields[@]}
    local i
    for ((i = 1; i <= count; ++i)); do
        header+=("col$i")
    done
}

update_widths() {
    local -n values_ref=$1
    local i width
    for i in "${!values_ref[@]}"; do
        width=${#values_ref[$i]}
        if ((width < 10)); then
            width=10
        fi
        if [[ -z ${widths[$i]+set} || $width -gt ${widths[$i]} ]]; then
            widths[$i]=$width
        fi
    done
}

print_separator() {
    local i j
    for i in "${!header[@]}"; do
        if ((i > 0)); then
            printf '  '
        fi
        for ((j = 0; j < widths[$i]; ++j)); do
            printf '-'
        done
    done
    printf '\n'
}

print_header() {
    update_widths header

    local i
    for i in "${!header[@]}"; do
        if ((i > 0)); then
            printf '  '
        fi
        printf '%*s' "${widths[$i]}" "${header[$i]}"
    done
    printf '\n'
    print_separator
}

print_row() {
    update_widths fields

    local i value
    for i in "${!header[@]}"; do
        if ((i > 0)); then
            printf '  '
        fi
        value=${fields[$i]:-}
        printf '%*s' "${widths[$i]}" "$value"
    done
    printf '\n'
}

if [[ $# -lt 1 || ${1:-} == "-h" || ${1:-} == "--help" ]]; then
    usage
    if [[ $# -lt 1 ]]; then
        exit 1
    fi
    exit 0
fi

reader=$1
shift

if [[ $reader == */* ]]; then
    if [[ ! -x $reader ]]; then
        echo "format.sh: '$reader' is not executable" >&2
        exit 1
    fi
else
    if ! reader_path=$(command -v -- "$reader"); then
        echo "format.sh: '$reader' was not found in PATH" >&2
        exit 1
    fi
    reader=$reader_path
fi

reader_cmd=("$reader")
if command -v stdbuf >/dev/null 2>&1; then
    reader_cmd=(stdbuf -oL -eL "$reader")
fi

delimiter=
detected_delimiter=
header_printed=0
fields=()
header=()
widths=()

"${reader_cmd[@]}" "$@" 2> >(sed -u 's/^/# /' >&2) |
    while IFS= read -r raw_line || [[ -n $raw_line ]]; do
        line=$(trim "$raw_line")
        if [[ -z $line ]]; then
            continue
        fi
        if [[ ${FORMAT_DEBUG:-0} != 0 ]]; then
            status "raw stdout: $line"
        fi

        split_line "$line"
        if ((header_printed == 0)) && looks_like_header; then
            delimiter=$detected_delimiter
            header=("${fields[@]}")
            print_header
            header_printed=1
            continue
        fi

        if ((header_printed == 0)) && ((${#fields[@]} > 0)) && is_numeric "${fields[0]}"; then
            delimiter=$detected_delimiter
            init_generated_header
            print_header
            header_printed=1
        fi

        if ((header_printed == 0)); then
            status "$line"
            continue
        fi

        if ((${#fields[@]} != ${#header[@]})); then
            status "ignored non-table line: $line"
            continue
        fi

        print_row
    done
