#!/usr/bin/env bash

set -euo pipefail

print_error() {
	echo -e "\e[41m\e[97mError: $1\e[0m"
}

check_tool() {
	if ! command -v "$1" &>/dev/null; then
		print_error "$1 not found. Please install $1 before running this script."
		exit 1
	fi
}

check_tool xmake
check_tool clang-format
check_tool clang-tidy
check_tool cppcheck

src_files=$(find . \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \))

if [ -z "$src_files" ]; then
	print_error "Nenhum arquivo .c ou .cpp encontrado"
	exit 0
fi

if [ ! -f "compile_commands.json" ]; then
	echo "== Generate compile_commands =="
	xmake project -k compile_commands
fi

echo "== Running clang-format =="
echo "$src_files" | xargs clang-format -i

echo "== Running clang-tidy =="
echo "$src_files" | xargs clang-tidy -extra-arg=-IInclude

echo "== Running cppcheck =="
cppcheck . --check-config --enable=warning --enable=style --enable=performance --inconclusive --max-ctu-depth=10 --suppress=missingInclude --suppress=unmatchedSuppression
