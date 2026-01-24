local BuildUtils = require("Meta.Lib.build_utils")
local RunCommand = require("Meta.Lib.run_command")
local Toolchain = require("Meta.Lib.toolchain")
local PrintMessage = require("Meta.Lib.print_message")

print(">>> Building Custom Clang Toolchain <<<")

local install_prefix = Toolchain.BASE_DIR
os.execute("mkdir -p " .. Toolchain.BIN_DIR)
os.execute("mkdir -p build")

if not os.execute("test -d build/llvm") then
    print("Cloning LLVM project...")
    RunCommand("git clone https://github.com/llvm/llvm-project.git -b release/21.x build/llvm --depth 1")
end

local build_dir = "build/llvm/build_clang"
os.execute("mkdir -p " .. build_dir)

local cmake_cmd = string.format([[
    cd %s && cmake -G Ninja ../llvm \
    -DLLVM_ENABLE_PROJECTS="clang;lld" \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DLLVM_ENABLE_TERMINFO=OFF \
    -DLLVM_ENABLE_LIBEDIT=OFF \
    -DLLVM_ENABLE_ZLIB=OFF \
    -DLLVM_ENABLE_ZSTD=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="%s" \
    -DLLVM_DEFAULT_TARGET_TRIPLE="%s" \
    -DCLANG_DEFAULT_LINKER=lld \
    -DCLANG_DEFAULT_RTLIB=compiler-rt \
    -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
    -DLLVM_ENABLE_RUNTIMES="" \
    -DLLVM_PARALLEL_LINK_JOBS=1 \
    -DLLVM_OPTIMIZED_TABLEGEN=ON \
    -DLLVM_USE_LINKER=lld \
    -DLLVM_CCACHE_BUILD=ON
]], build_dir, install_prefix, Toolchain.TRIPLE)

if os.execute(cmake_cmd) then
    print("Compiling Clang & LLD (Polite Mode)...")
    local cpu_count = BuildUtils.get_polite_cpu_count()
    local cmd = string.format("cd %s && nice -n 19 ninja -j%d clang lld", build_dir, cpu_count)
    if RunCommand(cmd) then
        print("Installing Toolchain...")
        os.execute("cd " .. build_dir .. " && ninja install")
        
        -- Rename Clang
        local clang_path = Toolchain.BIN_DIR .. "/clang"
        local new_clang_path = Toolchain.BIN_DIR .. "/" .. Toolchain.TRIPLE .. "-clang"
        os.execute(string.format("mv %s %s", clang_path, new_clang_path))
        
        -- Rename LLD
        local lld_path = Toolchain.BIN_DIR .. "/ld.lld"
        local new_lld_path = Toolchain.BIN_DIR .. "/" .. Toolchain.TRIPLE .. "-ld.lld"
        os.execute(string.format("mv %s %s", lld_path, new_lld_path))
        
        PrintMessage(false, "Toolchain successfully installed in " .. install_prefix)
    end
end
