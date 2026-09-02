# Support EwokOS running in Browser Spec

## Why
Currently, EwokOS requires QEMU or actual hardware (Raspberry Pi, etc.) to run. Running it in a browser would lower the barrier for users to try it out and debug it, making it more accessible.

## What Changes
- Add a new platform target `wasm` in `kernel/platform/wasm`.
- Add a new machine target `browser` in `machine.browser`.
- Update build system to support compiling to WebAssembly (using Clang/LLVM wasm32 target).
- Implement a minimal Hardware Abstraction Layer (HAL) for the browser environment (UART -> console, Framebuffer -> Canvas).

## Impact
- **Affected specs**: None.
- **Affected code**:
    - `kernel/platform/`: New `wasm` directory.
    - `machine.browser/`: New directory.
    - `kernel/Makefile`: Likely needs adjustment to support `ARCH=wasm` if not generic enough.

## ADDED Requirements

### Requirement: Wasm Platform Support
The system SHALL support compiling the kernel core to `wasm32-unknown-unknown` (or similar).
- **File**: `kernel/platform/wasm/make.rule`
- **File**: `kernel/platform/wasm/arch/wasm/boot.c` (Entry point)
- **File**: `kernel/platform/wasm/arch/wasm/mmu_arch.c` (MMU implementation)

### Requirement: MMU Handling (Soft MMU)
We SHALL implement a Software MMU that manages the `WebAssembly.Memory` as physical memory.

#### Design
- **Physical Memory**: The `WebAssembly.Memory` instance (imported from JS) represents the physical RAM of the machine.
- **Page Tables**: The kernel SHALL maintain software page tables within this memory to track virtual-to-physical mappings.
- **Translation**: 
  - The `mmu_arch.c` will implement standard MMU operations (`map_pages`, `unmap_pages`, `v2p`).
  - Since Wasm accesses memory via offsets, we treat these offsets as "Physical Addresses".
  - For the kernel, we use **Identity Mapping** (Virtual Address = Physical Address/Offset).
  - For user processes, we allocate distinct regions in the `WebAssembly.Memory`. The "Soft MMU" tracks these regions.
- **Enforcement**: 
  - In this initial version, memory protection is **cooperative** (no hardware trap on violation).
  - The kernel's memory allocator (`kalloc`) will use the Soft MMU structures to avoid overlapping allocations.

### Requirement: Browser Machine Support
The system SHALL provide a `machine.browser` target that implements the Board Support Package (BSP) for the browser environment.
- **File**: `machine.browser/kernel/bsp/uart.c` SHALL map UART output to `console.log` (via Wasm imports).
- **File**: `machine.browser/kernel/bsp/start.c` SHALL initialize the environment.

### Requirement: Web Entry Point
The system SHALL provide an HTML/JS harness to load and run the compiled Wasm kernel.
- **File**: `machine.browser/index.html`
- **File**: `machine.browser/runtime.js`

## Implementation Details
- **Toolchain**: We will use `clang` with `--target=wasm32`.
- **Memory**: We will use a linear memory imported from JS.
- **Context Switching**: For the initial version, we might support only single-tasking or use Asyncify (if applicable) for context switching, but a basic "boot and print" is the primary goal.

## Limitations
- This initial support may not support full multi-threading or complex device drivers.
- Framebuffer support may be limited or text-only initially.
