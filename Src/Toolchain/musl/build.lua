#!/usr/bin/env lua

local MUSL_VERSION = "1.2.4"
local WORK_DIR = "toolchain_build/musl"
local SRC_DIR = WORK_DIR .. "/musl-" .. MUSL_VERSION

print("--- FKernel Musl Cross-Compilation ---")

-- 1. Setup
os.execute("mkdir -p " .. WORK_DIR)

-- 2. Download
if not os.execute("ls " .. WORK_DIR .. "/musl.tar.gz > /dev/null 2>&1") then
    print("Downloading Musl...")
    os.execute("wget https://musl.libc.org/releases/musl-" .. MUSL_VERSION .. ".tar.gz -O " .. WORK_DIR .. "/musl.tar.gz")
end

-- 3. Extract
if not os.execute("ls " .. SRC_DIR .. " > /dev/null 2>&1") then
    print("Extracting...")
    os.execute("tar -xf " .. WORK_DIR .. "/musl.tar.gz -C " .. WORK_DIR)
end

-- 4. Configure & Build (Placeholder)
-- Para compilar a Musl de verdade, precisamos de um compilador cross-gcc ou clang 
-- configurado com os nossos headers de syscall.
print("Configuring Musl for FKernel (x86_64)...")

-- O segredo aqui é passar o nosso include do Userland/lib como header do sistema
local include_path = debug.getinfo(1).source:match("@?(.*/)")
local absolute_userland_inc = os.getenv("PWD") .. "/Userland/lib"

-- Comando de configuração simplificado
local config_cmd = string.format([[ 
    cd %s && ./configure \
    ARCH=x86_64 \
    --prefix=/usr \
    --disable-shared \
    CC="clang --target=x86_64-fkernel -ffreestanding -I%s"
]], SRC_DIR, absolute_userland_inc)

print("Run the following to build Musl manually (Cross-compiler required):")
print(config_cmd)