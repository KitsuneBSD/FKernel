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

if ! command -v cmake >/dev/null 2>&1; then
	echo "cmake not found. Please install it before continuing."
	exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
	echo "ninja not found. Please install it before continuing."
	exit 1
fi

if ! command -v git >/dev/null 2>&1; then
	echo "git not found. Please install it before continuing."
	exit 1
fi

if ! command -v make >/dev/null 2>&1; then
	echo "make not found. Please install it before continuing."
	exit 1
fi

if [ ! -d "build/llvm" ]; then
	git clone https://github.com/llvm/llvm-project.git -b release/21.x build/llvm
fi

cd build/llvm
mkdir build && cd build
cmake -G Ninja ../llvm \
	-DLLVM_ENABLE_PROJECTS="clang" \
	-DLLVM_TARGETS_TO_BUILD="X86" \
	-DLLVM_ENABLE_TERMINFO=OFF \
	-DLLVM_ENABLE_LIBEDIT=OFF \
	-DLLVM_ENABLE_ZLIB=OFF \
	-DLLVM_ENABLE_ZSTD=OFF \
	-DLLVM_INCLUDE_TESTS=OFF \
	-DLLVM_INCLUDE_EXAMPLES=OFF \
	-DLLVM_INCLUDE_BENCHMARKS=OFF \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$PWD/Toolchain/" \
	-DLLVM_DEFAULT_TARGET_TRIPLE="x86_64-unknown-elf" \
	-DCLANG_DEFAULT_LINKER=lld \
	-DCLANG_DEFAULT_RTLIB=compiler-rt \
	-DCLANG_DEFAULT_CXX_STDLIB=libc++ \
	-DLLVM_ENABLE_RUNTIMES=""

ninja -j"$CORES" clang
ninja install

mv "$PWD/Toolchain/clang" "$PWD/Toolchain/fkernel-clang"

echo "Clang successfully compiled and moved to Toolchain!"
