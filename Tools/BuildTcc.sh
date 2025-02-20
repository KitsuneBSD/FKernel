set -eo pipefail

source "$(dirname "$0")/Lib/check_command.sh"
source "$(dirname "$0")/Lib/extract_repo_name.sh"
source "$(dirname "$0")/Lib/clone_repository.sh"

ARCH=x86_64
NPROCS=$(nproc)
TCC_REPO="https://github.com/TinyCC/TinyCC"
TCC_NAME=$(extract_repo_name "$TCC_REPO")

function compile {
	output="$(dirname "$0")/bin/$ARCH-tcc"

	printf "[INFO]: Starting compiling...\n"

	if [ -f "configure" ]; then
		"./configure" "--prefix=/usr/local --enable-static"
	fi

	if [ -f "Makefile" ]; then
		make cross-"$ARCH" -j"$NPROCS"
	fi

	if [ -f "$output" ]; then
		if [ ! -f "/usr/lib/x86_64-libtcc1.a" ]; then
			sudo mv "x86_64-libtcc1.a" /usr/lib/
		fi
		printf "[INFO]: Compilação concluída! Binário movido para 'bin/'.\n"
	fi
}

mkdir -p "$(dirname "$0")/bin"

check_command "git"
check_command "gcc"

clone_repository "$TCC_REPO"

compile && cd ..

if [ -d "$TCC_NAME" ]; then
	printf "[INFO]: Removendo diretório '%s'...\n" "$TCC_NAME"
	rm -rf "$TCC_NAME"
fi
