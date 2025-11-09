#!/usr/bin/env bash
set -e

if command -v nproc >/dev/null 2>&1; then
	CORES=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
	CORES=$(sysctl -n hw.ncpu)
else
	CORES=1
fi
echo "Detected $CORES cores"

mkdir -p Toolchain
mkdir -p build

if ! command -v git >/dev/null 2>&1; then
	echo "git not found. Please install it before continuing."
	exit 1
fi

if ! command -v make >/dev/null 2>&1; then
	echo "make not found. Please install it before continuing."
	exit 1
fi

if [ ! -d "build/lua" ]; then
	git clone https://github.com/lua/lua build/lua
fi

cd build/lua
make -j"$CORES"

mv -f lua ../../Toolchain/fkernel-lua

cd ../../
echo "Lua successfully compiled and moved to Toolchain!"
