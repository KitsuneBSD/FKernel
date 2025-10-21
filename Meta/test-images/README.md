Test images for FKernel

This directory contains helper scripts to generate sample disk images used to test the kernel's partition parsing and device registration.

Requirements on the host:
- qemu-img
- sfdisk (part of util-linux)
- sgdisk (part of gdisk) — for GPT
- dd
- parted (optional)
- mkfs.ext4 (optional, for creating a filesystem in partition)
- losetup (optional)

Scripts
- make_mbr_img.sh — creates a small raw image with an MBR and a single partition
- make_ebr_img.sh — creates an image with a primary MBR that contains an extended partition and a couple of logical partitions (EBR chain)
- make_gpt_img.sh — creates a GPT disk image with two partitions
- make_bsd_img.sh — creates an image with a simple BSD disklabel (best-effort; host tools for BSD label manipulation may be limited)
- run_in_qemu.sh — launches QEMU with the generated image attached as a virtio or IDE disk and the current kernel ISO

Usage

Generate an MBR image:

```bash
cd build/test-images
./make_mbr_img.sh
```

Generate all images:

```bash
cd build/test-images
./make_all.sh
```

Run the kernel with the MBR image attached as hda:

```bash
cd build
../build/test-images/run_in_qemu.sh --disk ../build/test-images/mbr.img
```

Notes
- The scripts try to use portable `sfdisk` and `sgdisk` commands. If your distribution lacks one of the tools, install it with your package manager.
- The BSD disklabel script is best-effort; generating BSD labels on Linux may require `bsdutils` or manual dd writes. See the script header for details.
