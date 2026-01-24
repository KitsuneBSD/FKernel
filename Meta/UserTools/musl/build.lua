#!/usr/bin/env lua

-- Script de construção da Musl LibC para FKernel
local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local MUSL_VERSION = "1.2.4"
local MUSL_NAME = "musl-" .. MUSL_VERSION
local MUSL_URL = "https://musl.libc.org/releases/" .. MUSL_NAME .. ".tar.gz"
local ROOT_DIR = os.getenv("PWD")
local BUILD_DIR = ROOT_DIR .. "/build/userland/musl"
local SYSROOT = ROOT_DIR .. "/build/sysroot"

local function clean_kernel_leakage()
    print("Ensuring Sysroot is free of Kernel property...")
    -- Remove any possible leaked kernel-internal property
    os.execute("rm -rf " .. SYSROOT .. "/include/LibC")
    os.execute("rm -rf " .. SYSROOT .. "/include/LibFK")
end

print("--- FKernel Musl Toolchain ---")

if OSInteract.FileExists(SYSROOT .. "/lib/libc.a") then
    clean_kernel_leakage()
    PrintMessage(false, "Musl is already installed in sysroot. Leakage purged.")
    os.exit(0)
end

os.execute("mkdir -p " .. BUILD_DIR)
os.execute("mkdir -p " .. SYSROOT)

local tarball = BUILD_DIR .. "/musl.tar.gz"
ArchiveUtils.download(MUSL_URL, tarball)
ArchiveUtils.extract(tarball, BUILD_DIR)

local src_dir = BUILD_DIR .. "/" .. MUSL_NAME
local CC = Toolchain.get_clang()
local AR = Toolchain.get_tool("llvm-ar", "ar")
local RANLIB = Toolchain.get_tool("llvm-ranlib", "ranlib")

print("Configuring Musl in " .. src_dir)

local cmd_configure = string.format(
    "cd %s && CC='%s --target=%s' AR=%s RANLIB=%s ./configure --target=x86_64 --prefix= --syslibdir=/lib --disable-shared --disable-gcc-wrapper",
    src_dir, CC, Toolchain.TRIPLE, AR, RANLIB
)

if os.execute(cmd_configure) then
    print("Compiling Musl...")
    BuildUtils.make(src_dir)
    
    print("Installing Musl to Sysroot (".. SYSROOT .. ")...")
    os.execute("cd " .. src_dir .. " && make install DESTDIR=" .. SYSROOT)
    
    clean_kernel_leakage()
    
    PrintMessage(false, "Musl installed successfully. Sysroot is clean and isolated.")
else
    PrintMessage(true, "Error configuring Musl.")
    os.exit(1)
end