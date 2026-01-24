#!/usr/bin/env lua

local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local OPENRC_VERSION = "0.52.1"
local OPENRC_NAME = "openrc-" .. OPENRC_VERSION
local OPENRC_URL = "https://github.com/OpenRC/openrc/archive/refs/tags/" .. OPENRC_VERSION .. ".tar.gz"
local ROOT_DIR = os.getenv("PWD")
local BUILD_DIR = ROOT_DIR .. "/build/userland/openrc"
local SYSROOT = ROOT_DIR .. "/build/sysroot"
local INITRD_DIR = ROOT_DIR .. "/build/initrd_root"

print("---" .. " FKernel OpenRC Toolchain ---")

-- Previous checks
if not OSInteract.CommandExists("meson") then
    PrintMessage(true, "Meson not found. OpenRC requires meson.")
    os.exit(1)
end

if not OSInteract.FileExists(SYSROOT .. "/lib/libc.a") then
    PrintMessage(true, "Musl libc.a not found in sysroot. Build musl first.")
    os.exit(1)
end

os.execute("mkdir -p " .. BUILD_DIR)

local tarball = BUILD_DIR .. "/openrc.tar.gz"
if not OSInteract.FileExists(tarball) then
    ArchiveUtils.download(OPENRC_URL, tarball)
end

local src_dir = BUILD_DIR .. "/" .. OPENRC_NAME
if not OSInteract.DirExists(src_dir) then
    ArchiveUtils.extract(tarball, BUILD_DIR)
end

local build_openrc_dir = BUILD_DIR .. "/build"

-- Generate Meson Cross-File
local cross_file = BUILD_DIR .. "/cross_file.txt"
local f = io.open(cross_file, "w")
f:write("[binaries]\n")
f:write("c = '" .. Toolchain.get_clang() .. "'\n")
f:write("cpp = '" .. Toolchain.get_clang() .. "++'\n")
f:write("ar = '" .. Toolchain.get_tool("llvm-ar", "ar") .. "'\n")
f:write("strip = '" .. Toolchain.get_tool("llvm-strip", "strip") .. "'\n")
f:write("pkgconfig = 'false'\n")
f:write("\n")
f:write("[properties]\n")
f:write("c_args = ['--target=", Toolchain.TRIPLE, "', '-isystem', '" .. SYSROOT .. "/include', '-D__fkernel__']\n")
f:write("cpp_args = ['--target=", Toolchain.TRIPLE, "', '-isystem', '" .. SYSROOT .. "/include', '-D__fkernel__']\n")
f:write("c_link_args = ['-L" .. SYSROOT .. "/lib', '-static']\n")
f:write("cpp_link_args = ['-L" .. SYSROOT .. "/lib', '-static']\n")
f:write("\n")
f:write("[host_machine]\n")
f:write("system = 'linux'\n")
f:write("cpu_family = 'x86_64'\n")
f:write("cpu = 'x86_64'\n")
f:write("endian = 'little'\n")
f:close()

print("Configuring OpenRC with Meson...")
-- OpenRC options for minimal freestanding system
local cmd_setup = string.format(
    "cd %s && meson setup %s --cross-file %s --prefix=/ --libdir=lib --sysconfdir=/etc " ..
    "-Ddefault_library=static -Dshell=/bin/sh -Dpam=false -Dselinux=false -Daudit=false " ..
    "-Dbranding='FKernel' -Dsysvinit=false -Dnewnet=true -Dtermcap=false",
    src_dir, build_openrc_dir, cross_file
)

if not OSInteract.DirExists(build_openrc_dir) then
    if not os.execute(cmd_setup) then
        PrintMessage(true, "Failed to configure OpenRC")
        os.exit(1)
    end
end

print("Compiling OpenRC...")
if os.execute("cd " .. build_openrc_dir .. " && ninja") then
    print("Installing OpenRC to initrd staging via DESTDIR...")
    -- Use ninja install with DESTDIR to get everything (binaries + scripts + config)
    if os.execute(string.format("cd %s && DESTDIR=%s ninja install", build_openrc_dir, INITRD_DIR)) then
        -- Post-install adjustments
        os.execute("mv " .. INITRD_DIR .. "/sbin/openrc-init " .. INITRD_DIR .. "/sbin/init.openrc")
        PrintMessage(false, "OpenRC successfully installed to initrd.")
    else
        PrintMessage(true, "Failed to install OpenRC")
        os.exit(1)
    end
else
    PrintMessage(true, "Failed to compile OpenRC")
    os.exit(1)
end