#!/usr/bin/env bash

KERNEL_BIN=$(dirname "$0")/../Build/kernel.bin

grub-file --is-x86-multiboot "$KERNEL_BIN"

if [ $? ]; then
	echo "[OK]: Is a multiboot compatible"
else
	echo "[Fail]: Isn't a multiboot compatible"
fi
