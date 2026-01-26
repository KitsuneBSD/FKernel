#!/usr/bin/env lua

-- Script de construção da LibFKUser (Syscall wrappers)
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local RunCommand = require("Meta.Lib.run_command")

-- Infer project root directory
local ROOT_DIR = os.getenv("PWD") or "."
local BUILD_DIR = ROOT_DIR .. "/build/userland/libfk_user"
local SYSROOT = ROOT_DIR .. "/build/sysroot"

print("--- FKernel LibFKUser Build ---")

RunCommand("mkdir -p " .. BUILD_DIR)

local src_dir = "Src/Userland/lib"
local nasm_cmd = "nasm -f elf64 -i " .. ROOT_DIR .. "/ "

print("Compiling syscalls.asm...")
if not RunCommand(nasm_cmd .. src_dir .. "/syscalls.asm -o " .. BUILD_DIR .. "/syscalls.o") then
    PrintMessage(true, "Failed to compile syscalls.asm")
    os.exit(1)
end

print("Creating libfk_user.a...")
local AR = Toolchain.get_tool("llvm-ar", "ar")
if not RunCommand(AR .. " rcs " .. SYSROOT .. "/lib/libfk_user.a " .. BUILD_DIR .. "/syscalls.o") then
    PrintMessage(true, "Failed to create libfk_user.a")
    os.exit(1)
end

print("Installing libfk_user.a...")
RunCommand("mkdir -p " .. SYSROOT .. "/lib")
-- Library is already created in the previous step directly in SYSROOT/lib
-- if the AR command was successful.

PrintMessage(false, "LibFKUser built and installed successfully.")
