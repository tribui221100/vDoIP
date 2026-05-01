# Minimal DoIP Study Project (C/C++)

This project is a small educational setup to study **Diagnostics over IP (DoIP, ISO 13400)** using:

- A simple **DoIP server** written in C (`doip_server`)
- A simple **DoIP client** written in C++ (`doip_client`)

It is designed so you can run one side on host and one side inside a **QEMU Linux guest**.

## What is implemented

The project implements a minimal subset of DoIP over TCP port `13400`:

- DoIP generic header (version / inverse / payload type / payload length)
- Vehicle Identification Request (`0x0001`, empty payload)
- Vehicle Identification Response (`0x0004`)
- Routing Activation Request (`0x0005`)
- Routing Activation Response (`0x0006`)

## Project layout

```text
.
├── Makefile
├── README.md
└── src
    ├── doip_client.cpp
    ├── doip_common.h
    └── doip_server.c
```

## Build

```bash
make
```

This creates:

- `build/doip_server`
- `build/doip_client`

## Run locally (host only)

Terminal 1:

```bash
./build/doip_server
```

Terminal 2:

```bash
./build/doip_client 127.0.0.1
```

## QEMU Setup (macOS Apple Silicon)

Use this ARM64 image path:

`/Users/bmt2211/Documents/qemu_img/ubuntu_arm64.qcow2`

You can start the VM with the provided script:

```bash
./scripts/run_vm.sh
```

## Run with QEMU guest

### 1) Start QEMU with port forward

Use user-mode networking and forward host port `13400` to guest `13400`:

```bash
qemu-system-aarch64 \
  -machine virt,accel=hvf \
  -cpu host \
  -smp 2 \
  -m 1024 \
  -drive if=virtio,file="/Users/bmt2211/Documents/qemu_img/ubuntu_arm64.qcow2",format=qcow2 \
  -nic user,hostfwd=tcp::13400-:13400
```

### 2) In guest, run server

Copy binaries (or build inside guest), then:

```bash
./doip_server
```

### 3) On host, run client

Connect to forwarded port:

```bash
./build/doip_client 127.0.0.1 13400
```

You should see:

- client sends vehicle identification request
- server responds with a vehicle identification response
- client sends routing activation request
- server responds with routing activation response

## Reverse direction (host server, guest client)

If server runs on host:

```bash
./build/doip_server 0.0.0.0 13400
```

From guest, connect to host default gateway in user-mode networking (commonly `10.0.2.2`):

```bash
./doip_client 10.0.2.2 13400
```

## Notes

- This is a **study/demo** implementation, not full ISO 13400 compliance.
- No TLS, UDP discovery, or advanced diagnostics payload handling is included.
