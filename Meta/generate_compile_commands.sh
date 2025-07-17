#!/usr/bin/env bash

print_error() {
	echo -e "\e[41m\e[97mError: $1\e[0m"
}

print_info() {
	echo -e "\e[44m\e[97mInfo:\e[0m $1"
}

check_tool() {
	if ! command -v "$1" &>/dev/null; then
		print_error "$1 not found. Please install $1 before running this script."
		exit 1
	fi
}

generate_compile_commands() {
	print_info "Generating compile_commands.json..."
	if [ -f ".xmake/linux/x86_64/project.lock" ]; then
		rm ".xmake/linux/x86_64/project.lock"
	fi

	xmake project -k compile_commands --yes -vD
	if [ $? -ne 0 ]; then
		print_error "xmake failed to generate compile_commands.json"
		exit 1
	fi
	if [ ! -f "compile_commands.json" ]; then
		print_error "compile_commands.json was not created"
		exit 1
	fi
}

sanitize_compile_commands() {
	print_info "Sanitizing compile_commands.json..."
	tmp_file=$(mktemp)
	jq 'map(
        .arguments |= map(
            select(. != "-f" and . != "-w-label-orphan" and . != "-w-other")
        )
    )' compile_commands.json >"$tmp_file" && mv "$tmp_file" compile_commands.json
	if [ $? -ne 0 ]; then
		print_error "Failed to sanitize compile_commands.json"
		exit 1
	fi
}

check_tool xmake
check_tool jq

if [ ! -f "compile_commands.json" ]; then
	generate_compile_commands
else
	print_info "compile_commands.json already exists"
fi

sanitize_compile_commands
print_info "compile_commands.json is ready for clang-tidy"
