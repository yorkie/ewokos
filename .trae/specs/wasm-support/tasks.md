# Tasks

- [x] Task 1: Create `kernel/platform/wasm` Support
  - [x] SubTask 1.1: Create `kernel/platform/wasm/make.rule` defining `ARCH=wasm` and `CROSS_COMPILE=clang --target=wasm32`.
  - [x] SubTask 1.2: Create `kernel/platform/wasm/arch/wasm/boot.c` (or `boot.S`) as entry point.
  - [x] SubTask 1.3: Create `kernel/platform/wasm/arch/wasm/core.c` for context switching (stub initially).
  - [x] SubTask 1.4: Create `kernel/platform/wasm/arch/wasm/irq.c` and `timer.c` (stubs initially).
  - [x] SubTask 1.5: Implement `kernel/platform/wasm/arch/wasm/mmu_arch.c` as a Soft MMU (managing `WebAssembly.Memory` regions).

- [x] Task 2: Create `machine.browser` Support
  - [x] SubTask 2.1: Create `machine.browser/kernel/bsp/` directory.
  - [x] SubTask 2.2: Implement `machine.browser/kernel/bsp/uart.c` mapping to JS console.
  - [x] SubTask 2.3: Implement `machine.browser/kernel/bsp/start.c` for initialization.
  - [x] SubTask 2.4: Create `machine.browser/kernel/Makefile` based on `machine.virt/kernel/Makefile` but targeting Wasm.

- [x] Task 3: Create Web Harness
  - [x] SubTask 3.1: Create `machine.browser/index.html` to load the Wasm module.
  - [x] SubTask 3.2: Create `machine.browser/runtime.js` to provide imports (env, uart, etc.).

- [x] Task 4: Verify Build and Run
  - [x] SubTask 4.1: Run `make` in `machine.browser/kernel`.
  - [x] SubTask 4.2: Serve `machine.browser` directory and verify output in browser console (Server running at http://localhost:8000).
