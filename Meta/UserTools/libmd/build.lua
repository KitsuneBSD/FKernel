#!/usr/bin/env lua

-- Script de construção da LibMD para FKernel (Dependência da LibBSD)
local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local LIBMD_VERSION = "1.1.0"
local LIBMD_NAME = "libmd-" .. LIBMD_VERSION
local LIBMD_URL = "https://archive.hadrons.org/software/libmd/" .. LIBMD_NAME .. ".tar.xz"
local ROOT_DIR = os.getenv("PWD")
local BUILD_DIR = ROOT_DIR .. "/build/userland/libmd"
local SYSROOT = ROOT_DIR .. "/build/sysroot"

print("--- FKernel LibMD Build ---")

if OSInteract.FileExists(SYSROOT .. "/lib/libmd.a") then
    PrintMessage(false, "LibMD is already installed. Skipping.")
    os.exit(0)
end

os.execute("mkdir -p " .. BUILD_DIR)

local tarball = BUILD_DIR .. "/libmd.tar.xz"
ArchiveUtils.download(LIBMD_URL, tarball)
ArchiveUtils.extract(tarball, BUILD_DIR)

local src_dir = BUILD_DIR .. "/" .. LIBMD_NAME
local CC = Toolchain.get_clang()

print("Configuring LibMD...")
local cmd_configure = string.format(
    "cd %s && CC='%s --target=%s --sysroot=%s' ./configure --host=x86_64-fkernel --prefix= --disable-shared",
    src_dir, CC, Toolchain.TRIPLE, SYSROOT
)

if os.execute(cmd_configure) then
    print("Compiling LibMD...")
    BuildUtils.make(src_dir)
    print("Installing LibMD...")
    os.execute("cd " .. src_dir .. " && make install DESTDIR=" .. SYSROOT)
    PrintMessage(false, "LibMD installed successfully.")
else
    PrintMessage(true, "Failed to configure LibMD.")
    os.exit(1)
end
