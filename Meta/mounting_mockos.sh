#!/usr/bin/bash

print_error() {
	echo -e "\e[41m\e[97mError: $1\e[0m"
}

if ! command -v grub-mkrescue &>/dev/null; then
	print_error "grub-mkrescue not found. Please install grub-mkrescue before running this script."
	exit 1
fi

grub_directory="build/mockos/boot/grub"
grub_config="Config/grub.cfg"
kernel_binary="build/FKernel.bin"

if [ ! -d "$grub_directory" ]; then
	mkdir -p "$grub_directory"
else
	rm -rf "$grub_directory" && mkdir -p "$grub_directory"
fi

cp "$grub_config" "$grub_directory"
cp "$kernel_binary" "build/mockos/boot"

grub-mkrescue /usr/lib/grub/i386-pc/ -o build/FKernel-MockOS.iso "build/mockos"
