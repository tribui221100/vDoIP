#!/bin/bash

IMAGE_PATH="/Users/bmt2211/Documents/qemu_img/ubuntu_arm64.qcow2"
# Đường dẫn UEFI mặc định khi cài QEMU bằng Homebrew trên Mac M1/M2/M3
UEFI_PATH="/opt/homebrew/share/qemu/edk2-aarch64-code.fd"

echo "Starting vDoIP Guest Machine..."

qemu-system-aarch64 \
  -m 2G \
  -M virt,highmem=off \
  -cpu host \
  -accel hvf \
  -smp 2 \
  -drive if=pflash,format=raw,readonly=on,file="$UEFI_PATH" \
  -drive if=none,file="$IMAGE_PATH",id=hd0 \
  -device virtio-blk-device,drive=hd0 \
  -netdev user,id=net0,hostfwd=tcp::13400-:13400,hostfwd=tcp::2222-:22 \
  -device virtio-net-device,netdev=net0 \
  -display none \
  -boot menu=on,splash-time=5000 \
  -serial mon:stdio