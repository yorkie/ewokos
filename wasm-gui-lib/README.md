# EwokOS XWIN GUI Library - WebAssembly Port

This is a standalone WebAssembly port of the XWIN GUI library from EwokOS, designed to run in web browsers.

## Structure

- `src/` - Source code for the GUI library
  - `core/` - Core windowing system (X library port)
  - `cpp/` - C++ wrapper layer (X++ library port)
  - `widgets/` - High-level widget framework (Widget++ library port)
  - `web/` - WebAssembly/HTML5 integration layer
- `include/` - Header files
- `demo/` - Demo applications
- `build/` - Build output directory

## Dependencies

The library provides web-compatible replacements for:
- EwokOS system calls → Web APIs
- Graphics rendering → HTML5 Canvas
- Font rendering → Web fonts
- Memory management → WebAssembly memory model

## Building

```bash
make clean
make
```

## Usage

```html
<script src="ewok-gui.js"></script>
<canvas id="canvas" width="800" height="600"></canvas>
<script>
  const gui = new EwokGUI('canvas');
  const window = gui.createWindow(100, 100, 400, 300);
  // ... use GUI components
</script>
```