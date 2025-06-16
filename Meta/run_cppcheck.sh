#!/usr/bin/env bash

print_error() {
	echo -e "\e[41m\e[97mError: $1\e[0m"
}

start_cppcheck() {
	cppcheck . --check-config --enable=warning --enable=style --enable=performance --inconclusive --max-ctu-depth=10 --suppress=missingInclude --suppress=unmatchedSuppression
}

if ! command -v cppcheck &>/dev/null; then
	print_error "cppcheck not found. Please install cppcheck before running this script."
	exit 1
fi

cpp_files=$(find . -name "*.cpp")
c_files=$(find . -name "*.c")

if [ -z "$cpp_files" ] && [ -z "$c_files" ]; then
	print_error "Nenhum c/cpp file foi encontrado"
	exit 0
fi

if [ -n "$c_files" ]; then
	start_cppcheck "$c_files"
fi

if [ -n "$cpp_files" ]; then
	start_cppcheck "$cpp_files"
fi
