set -eo pipefail

. "./Lib/set_path.sh"

. "./Lib/check_command.sh"
. "./Lib/extract_repo_name.sh"
. "./Lib/clone_repository.sh"

ARCH=x86_64
NPROCS=$(nproc)
TCC_REPO="https://github.com/TinyCC/TinyCC"
TCC_NAME=$(extract_repo_name "$TCC_REPO")

function compile {
	output="$ARCH-tcc"

	printf "[INFO]: Starting compiling...\n"

	if [ -f "./configure" ]; then
		"./configure"
	fi

	if [ -f "Makefile" ]; then
		make cross-"$ARCH" -j"$NPROCS"
	fi

	if [ -f "$output" ]; then
		mv "$output" "../bin"
		printf "[INFO]: Compilação concluída! Binário movido para 'bin/'.\n"
	else
		printf "[ERROR]: Output file '%s' not found after compilation.\n" "$output"
		exit 1
	fi
}

mkdir -p "bin"

check_command "git"
check_command "gcc"

clone_repository "$TCC_REPO"

compile && cd ..

if [ -d "$TCC_NAME" ]; then
	printf "[INFO]: Removendo diretório '%s'...\n" "$TCC_NAME"
	rm -rf "$TCC_NAME"
fi
