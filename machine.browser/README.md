# EwokOS native WebAssembly port

This target compiles the EwokOS kernel and userspace sources directly to
`wasm32`. It contains no QEMU, v86, guest CPU instruction decoder, or native
ARM/RISC-V executable fallback.

## Build and run

```sh
make -C machine.browser test
make -C machine.browser serve
```

Open <http://127.0.0.1:8765/>. Click the framebuffer to focus keyboard and
pointer input. Kernel and TTY output remain visible below the framebuffer; the
interactive EwokOS shell accepts built-ins such as `pwd`, `cd`, `export`, and
`history`.

Use the `serve` target rather than a generic static server. It emits COOP/COEP
headers so the browser can create shared WebAssembly memory and Worker threads.

The browser creates one threaded, shared 896 MiB WebAssembly linear memory. The native
Wasm kernel owns a 128 MiB physical-memory model and implements per-process
virtual mappings with EwokOS two-level page tables backed by a software MMU.
Every userspace module has a disjoint linked arena which is registered in its
EwokOS process page table. libc/libgloss syscalls enter the existing kernel
`svc_handler`; IPC context switches are dispatched into the owning Wasm
module, without interpreting machine instructions.

The kernel, syscall dispatcher, device services, and userspace scheduler run in
the `ewokos-kernel` Worker. A separate `ewokos-compositor` Worker reads the
shared framebuffer, performs BGRA conversion, and transfers display frames to
the browser UI. The main thread only handles browser input, audio playback, and
the final canvas presentation, so long guest frames cannot block the page UI.
Leaf GUI programs such as xDemo, xcores, xprocs, clock, and xlog run in dedicated
Process Workers. Their synchronous EwokOS syscalls cross an Atomics mailbox to
the Kernel Worker, which keeps kernel and service IPC ordering deterministic
while application code can execute on separate browser cores.

## Booted system

`user/modules.json` is the boot manifest and Wasm executable registry. The
browser target currently boots the real EwokOS implementations of:

- init, core, sessiond, vfsd and sdfsd;
- ext3 rootfs, browser TTY, null, ramfs, pipe and timer devices;
- display manager, framebuffer display, g2dd, fontd, FreeType and libpng/zlib;
- keybd, moused, X server, XIM, XWM, xmouse, splash and xDemo;
- logd, soundd/WebAudio, timed, powerd, ethd and the native IPv4/TCP/UDP stack;
- shell plus representative `cat` and `uname` programs and boot probes.

The real init launches the shell. The shell first executes
`/etc/init.wasm.rd` through ext3/VFS and then switches to cooperative
interactive TTY input. Host-backed input devices are marked as interrupt
pollers; ordinary processes only step when the EwokOS kernel reports them
runnable.

The G2D Wasm scalar backend implements clipped fill, source-alpha blit,
nearest-neighbor scale, and clockwise rotation (including arbitrary integer
angles). Its browser boot probe exercises every operation through `/dev/g2d`
and keyed EwokOS shared memory.

## Storage and networking

`sdfsd` mounts `rootfs.img` as ext3 using the existing EwokOS block and ext3
code. Browser sector writes are persisted in IndexedDB and restored on the
next page load when the bundled image size matches. The boot probe increments
`/etc/wasm-boot-count` and fsyncs it to verify persistence.

`ethd` exposes `/dev/eth0`; `netd` mounts `/dev/net0` with loopback
`127.0.0.1/8` and Ethernet `10.0.2.15/24`. The boot probe sends a real UDP
datagram through the EwokOS socket API and loopback stack, then emits Ethernet
ARP traffic. By default outbound frames are counted and dropped at the
browser boundary. To bridge raw Ethernet frames to an external service, pass
one binary-frame-per-message WebSocket endpoint:

```text
http://127.0.0.1:8765/?netRelay=wss://relay.example/ethernet
```

The relay is an optional layer-2 transport, not a CPU or device emulator.

## Host ABI and boundary

The kernel imports shared memory, monotonic time, console output, block reads,
halt and ready notifications. Userspace imports the syscall gateway plus
minimal browser device operations for block storage, TTY, framebuffer,
keyboard, pointer, audio, wall-clock time, Ethernet frames, and registry-based
Wasm process spawning. `test-kernel.mjs` rejects WASI and undocumented imports.

Native hardware-specific drivers that require privileged MMIO, a physical GPU,
or a board controller must be replaced by a browser BSP to be meaningful in a
browser. That boundary is implemented as source-level Wasm devices; the target
never executes or emulates a foreign CPU image.
