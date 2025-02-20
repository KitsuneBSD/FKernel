set -eo pipefail

source "$(dirname "$0")/Lib/check_command.sh"
source "$(dirname "$0")/Lib/extract_repo_name.sh"
source "$(dirname "$0")/Lib/clone_repository.sh"

ARCH=x86_64
NPROCS=$(nproc)
TCC_REPO="https://github.com/TinyCC/TinyCC"
TCC_NAME=$(extract_repo_name "$TCC_REPO")

function compile {
	output="$ARCH-tcc"

	echo "$PWD"

	printf "[INFO]: Starting compiling...\n"

	if [ -f "configure" ]; then
		"./configure" "--prefix=/usr/local --enable-static"
	fi

	if [ -f "Makefile" ]; then
		make cross-"$ARCH" -j"$NPROCS"
	fi

	if [ -f "$output" ]; then
		mv "$output" "../bin"
		sudo mv "x86_64-libtcc1.a" /usr/lib/
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
