#!/usr/bin/env lua

local BB_VERSION = "1.36.1"
local WORK_DIR = "toolchain_build/busybox"

print("--- FKernel BusyBox (ash shell) Downloader ---")
os.execute("mkdir -p " .. WORK_DIR)

if not os.execute("ls " .. WORK_DIR .. "/busybox.tar.bz2 > /dev/null 2>&1") then
    print("Downloading BusyBox...")
    os.execute("wget https://busybox.net/downloads/busybox-" .. BB_VERSION .. ".tar.bz2 -O " .. WORK_DIR .. "/busybox.tar.bz2")
    os.execute("tar -xf " .. WORK_DIR .. "/busybox.tar.bz2 -C " .. WORK_DIR)
end

print("BusyBox is ready for compilation against Musl/FKernel.")
