
-- Script de construção da LibBSD (Official) para FKernel
local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local RunCommand = require("Meta.Lib.run_command")

local LIBBSD_VERSION = "0.11.7"
local LIBBSD_NAME = "libbsd-" .. LIBBSD_VERSION
local LIBBSD_URL = "https://libbsd.freedesktop.org/releases/" .. LIBBSD_NAME .. ".tar.xz"

-- Infer project root directory
local ROOT_DIR = os.getenv("PWD") or "."
local BUILD_DIR = ROOT_DIR .. "/build/userland/libbsd"
local SYSROOT = ROOT_DIR .. "/build/sysroot"

print("--- FKernel Official LibBSD Build ---")

-- Ensure libmd is built first
if not OSInteract.FileExists(SYSROOT .. "/lib/libmd.a") then
    print("Building LibMD dependency first...")
    if not RunCommand("lua Meta/UserTools/libmd/build.lua") then
        PrintMessage(true, "Failed to build LibMD dependency.")
        os.exit(1)
    end
end

if OSInteract.FileExists(SYSROOT .. "/lib/libbsd.a") then
    PrintMessage(false, "Official LibBSD is already installed. Skipping.")
    os.exit(0)
end

RunCommand("mkdir -p " .. BUILD_DIR)

local tarball = BUILD_DIR .. "/libbsd.tar.xz"
ArchiveUtils.download(LIBBSD_URL, tarball)
ArchiveUtils.extract(tarball, BUILD_DIR)

local src_dir = BUILD_DIR .. "/" .. LIBBSD_NAME
local CC = Toolchain.get_clang()

print("Configuring LibBSD...")
-- LibBSD might need LIBMD_CFLAGS and LIBMD_LIBS if pkg-config is not set up correctly for cross-compiling
local cmd_configure = string.format(
    "cd %s && CC='%s --target=%s --sysroot=%s' LIBMD_CFLAGS='-I%s/include' LIBMD_LIBS='-L%s/lib -lmd' ./configure --host=x86_64-linux-musl --prefix= --disable-shared",
    src_dir, CC, Toolchain.TRIPLE, SYSROOT, SYSROOT, SYSROOT
)

if RunCommand(cmd_configure) then
    print("Compiling LibBSD...")
    BuildUtils.make(src_dir)
    print("Installing LibBSD...")
    RunCommand("cd " .. src_dir .. " && make install DESTDIR=" .. SYSROOT)
    PrintMessage(false, "Official LibBSD installed successfully.")
else
    PrintMessage(true, "Failed to configure LibBSD.")
    os.exit(1)
end
