#!/usr/bin/env bash
set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <number_of_cores>"
    exit 1
fi
CORES=$1
echo "Using $CORES cores"

mkdir -p Toolchain
mkdir -p build

if ! command -v git >/dev/null 2>&1; then
	echo "git not found. Please install it before continuing."
	exit 1
fi

if ! command -v autoconf >/dev/null 2>&1; then
	echo "autoconf not found. Please install it before continuing"
	exit 1
fi

if ! command -v make >/dev/null 2>&1; then
	echo "make not found. Please install it before continuing."
	exit 1
fi

if [ ! -d "build/nasm" ]; then
	git clone https://github.com/netwide-assembler/nasm build/nasm
fi

cd build/nasm
./autogen.sh
./configure
make -j"$CORES"

mv -f nasm ../../Toolchain/fkernel-nasm

cd ../../
echo "Nasm successfully compiled and moved to Toolchain!"
