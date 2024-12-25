.PHONY: all qemu

DirectoryList=Src/Loader

all:
	for vdir in $(DirectoryList); do \
		$(MAKE) -C $$vdir; \
	done

qemu: Build/Loader/boot.bin
	qemu-system-x86_64 -hda Build/Loader/boot.bin
