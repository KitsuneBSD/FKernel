#!/usr/bin/env lua

-- Script de construção do BusyBox para FKernel
local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local BB_VERSION = "1.36.1"
local BB_NAME = "busybox-" .. BB_VERSION
local BB_URL = "https://busybox.net/downloads/" .. BB_NAME .. ".tar.bz2"
local ROOT_DIR = os.getenv("PWD")
local BUILD_DIR = ROOT_DIR .. "/build/userland/busybox"
local SYSROOT = ROOT_DIR .. "/build/sysroot"
local INITRD_DIR = ROOT_DIR .. "/build/initrd_root"

print("--- FKernel BusyBox Toolchain ---")

local function mlink(target, link)
    os.execute(string.format("ln -sf %s %s/%s", target, INITRD_DIR, link))
end

local function create_etc_configs()
    print("Creating basic /etc configurations...")
    os.execute("mkdir -p " .. INITRD_DIR .. "/etc/init.d")
    
    -- Create /etc/inittab
    -- local inittab = io.open(INITRD_DIR .. "/etc/inittab", "w")
    -- if inittab then
    --     inittab:write("::sysinit:/etc/init.d/rcS\n")
    --     inittab:write("::askfirst:-/bin/sh\n")
    --     inittab:write("::ctrlaltdel:/sbin/reboot\n")
    --     inittab:write("::shutdown:/bin/umount -a -r\n")
    --     inittab:close()
    -- end

    -- Create /etc/init.d/rcS
    local rcs = io.open(INITRD_DIR .. "/etc/init.d/rcS", "w")
    if rcs then
        rcs:write("#!/bin/sh\n")
        rcs:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
        rcs:write("mount -t proc proc /proc\n")
        rcs:write("echo \"FKernel standard system booted successfully.\"\n")
        rcs:close()
        os.execute("chmod +x " .. INITRD_DIR .. "/etc/init.d/rcS")
    end

    -- Create /etc/profile
    local profile = io.open(INITRD_DIR .. "/etc/profile", "w")
    if profile then
        profile:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
        profile:write("export PS1='# '\n")
        profile:close()
    end
end

local function create_all_links()
    print("Creating safe symlinks for initrd...")
    create_etc_configs()
    mlink("busybox", "bin/sh")
    mlink("busybox", "bin/echo")
    mlink("busybox", "bin/mkdir")
    mlink("busybox", "bin/rm")
    mlink("busybox", "bin/cp")
    mlink("busybox", "bin/mv")
    mlink("busybox", "bin/pwd")
    mlink("busybox", "bin/clear")
    mlink("busybox", "bin/mount")
    mlink("busybox", "bin/umount")
    mlink("/bin/busybox", "sbin/init")
end

local src_dir = BUILD_DIR .. "/" .. BB_NAME
if OSInteract.FileExists(src_dir .. "/busybox") then
    PrintMessage(false, "BusyBox is already compiled. Skipping build.")
    os.execute("mkdir -p " .. INITRD_DIR .. "/bin")
    os.execute("mkdir -p " .. INITRD_DIR .. "/sbin")
    os.execute("cp " .. src_dir .. "/busybox " .. INITRD_DIR .. "/bin/")
    create_all_links()
    os.exit(0)
end

os.execute("mkdir -p " .. BUILD_DIR)
os.execute("mkdir -p " .. INITRD_DIR .. "/bin")
os.execute("mkdir -p " .. INITRD_DIR .. "/sbin")

local tarball = BUILD_DIR .. "/busybox.tar.bz2"
ArchiveUtils.download(BB_URL, tarball)
ArchiveUtils.extract(tarball, BUILD_DIR)

if not OSInteract.FileExists(src_dir .. "/.config") then
    print("Generating minimal configuration...")
    os.execute("make -C " .. src_dir .. " allnoconfig")
    local f = io.open(src_dir .. "/.config", "a")
    if f then
        f:write("CONFIG_STATIC=y\n")
        f:write("CONFIG_PLATFORM_LINUX=y\n")
        f:write("CONFIG_HUSH=y\n")
        f:write("CONFIG_INIT=y\n")
        f:write("CONFIG_ECHO=y\n")
        f:write("CONFIG_MKDIR=y\n")
        f:write("CONFIG_RM=y\n")
        f:write("CONFIG_CP=y\n")
        f:write("CONFIG_MV=y\n")
        f:write("CONFIG_PWD=y\n")
        f:write("CONFIG_MOUNT=y\n")
        f:write("CONFIG_UMOUNT=y\n")
        f:close()
    end
    os.execute("make -C " .. src_dir .. " oldconfig")
end

print("Compiling BusyBox...")
local CC = Toolchain.get_clang()
local cflags = string.format("--sysroot=%s -isystem %s/include --target=%s", SYSROOT, SYSROOT, Toolchain.TRIPLE)
local ldflags = string.format("-static --sysroot=%s -L%s/lib", SYSROOT, SYSROOT)

local make_args = {
    string.format("CC='%s'", CC),
    string.format("EXTRA_CFLAGS='%s'", cflags),
    string.format("EXTRA_LDFLAGS='%s'", ldflags)
}

if BuildUtils.make(src_dir, make_args) then
    os.execute("cp " .. src_dir .. "/busybox " .. INITRD_DIR .. "/bin/")
    create_all_links()
    PrintMessage(false, "BusyBox installed safely.")
else
    os.exit(1)
end
