#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Generate .qrc from all files in the directory.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

DIR="$(pwd)"
OUTPUT="resources.qrc"

usage() {
	echo "Usage: $(basename "$0") [options]"
	echo ""
	echo "Options:"
	echo "  -h, --help          Print this help message."
	echo "  -d, --dir <path>    Directory to scan for resources, default: current directory."
	echo "  -o, --output <path> Output file, default: 'resources.qrc'."
}

parse-args() {
	while [[ $# -gt 0 ]]; do
		case "$1" in
		-h | --help)
			usage
			exit 0
			;;
		-d | --dir)
			DIR="$2"
			shift 2
			;;
		--dir=*)
			DIR="${1#*=}"
			shift 1
			;;
		-o | --output)
			OUTPUT="$2"
			shift 2
			;;
		--output=*)
			OUTPUT="${1#*=}"
			shift 1
			;;
		*)
			echo "Invalid argument: $1."
			usage
			exit 1
			;;
		esac
	done
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

gen-qrc() {
	cat >"$OUTPUT" <<EOF
<!DOCTYPE RCC>
<RCC version="1.0">
  <qresource>
EOF

	local FILE
	find "$DIR" -type f | while read -r FILE; do
		local REL_PATH ABS_PATH
		REL_PATH="/${FILE#$DIR/}"
		ABS_PATH="$(realpath "$FILE")"
		echo "    <file alias=\"$REL_PATH\">$ABS_PATH</file>" >>"$OUTPUT"
	done

	cat >>"$OUTPUT" <<EOF
  </qresource>
</RCC>
EOF
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

parse-args "$@"
gen-qrc

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
