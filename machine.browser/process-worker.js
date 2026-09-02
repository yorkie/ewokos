let instance = null;
let mailbox = null;
let pid = -1;
let pollIntervalMs = 100;
let timer = 0;
let sharedMemory = null;
let launchArgument = "";
let launchGeneration = 0;
const encoder = new TextEncoder();
const WASM_HOSTCALL_SPAWN_COMMAND = -0x1001;
const wasmMathImports = Object.freeze({
  cos: Math.cos,
  sin: Math.sin,
  log: Math.log,
  rint: Math.round,
  exp: Math.exp,
  atan: Math.atan,
  floor: Math.floor,
  ldexp: (value, exponent) => value * Math.pow(2, exponent),
  pow: Math.pow,
  ceil: Math.ceil,
});

function copyLaunchArgument(ptr, capacity) {
  if (!sharedMemory || capacity <= 0) return 0;
  const bytes = encoder.encode(launchArgument);
  const copied = Math.min(bytes.length, capacity - 1);
  const target = new Uint8Array(sharedMemory.buffer, ptr, capacity);
  target.set(bytes.subarray(0, copied));
  target[copied] = 0;
  return copied;
}

function syscall(code, arg0, arg1, arg2) {
  Atomics.store(mailbox, 1, code);
  Atomics.store(mailbox, 2, arg0);
  Atomics.store(mailbox, 3, arg1);
  Atomics.store(mailbox, 4, arg2);
  Atomics.store(mailbox, 0, 1);
  postMessage({ type: "syscall" });
  while (Atomics.load(mailbox, 0) === 1)
    Atomics.wait(mailbox, 0, 1);
  const result = Atomics.load(mailbox, 5);
  Atomics.store(mailbox, 0, 0);
  return result;
}

function scheduleStep() {
  clearTimeout(timer);
  timer = setTimeout(() => {
    try {
      instance.exports.ewok_step();
      scheduleStep();
    } catch (error) {
      postMessage({ type: "error", message: error.stack || error.message });
    }
  }, pollIntervalMs);
}

self.addEventListener("message", async (event) => {
  const message = event.data;
  if (message.type === "prepare") {
    try {
      mailbox = new Int32Array(message.mailbox);
      sharedMemory = message.memory;
      pollIntervalMs = Math.max(16, message.pollIntervalMs || 100);
      const module = await WebAssembly.compileStreaming(fetch(message.url, { cache: "no-store" }));
      instance = await WebAssembly.instantiate(module, {
        env: {
          ...wasmMathImports,
          memory: message.memory,
          ewok_syscall: syscall,
          wasm_host_block_read: () => -1,
          wasm_host_block_write: () => -1,
          wasm_host_block_flush: () => 0,
          wasm_host_tty_write: () => 0,
          wasm_host_tty_read: () => 0,
          wasm_host_tty_available: () => 0,
          wasm_host_tty_input: () => 0,
          wasm_host_spawn_command: (ptr, len) =>
            syscall(WASM_HOSTCALL_SPAWN_COMMAND, ptr, len, 0),
          wasm_host_launch_argument: copyLaunchArgument,
          wasm_host_launch_generation: () => launchGeneration,
          wasm_host_framebuffer_configure: () => -1,
          wasm_host_framebuffer_flush: () => -1,
          wasm_host_key_read: () => 0,
          wasm_host_key_available: () => 0,
          wasm_host_mouse_read: () => 0,
          wasm_host_mouse_available: () => 0,
          wasm_host_audio_write: () => 0,
          wasm_host_unix_time_sec: () => Math.floor(Date.now() / 1000),
          wasm_host_net_read: () => 0,
          wasm_host_net_write: () => -1,
          wasm_host_net_available: () => 0,
        },
      });
      postMessage({
        type: "prepared",
        command: instance.exports.ewok_init_command(),
        commandSize: instance.exports.ewok_init_command_size(),
        moduleBase: instance.exports.ewok_module_base(),
        moduleSize: instance.exports.ewok_module_size(),
        heapBase: instance.exports.ewok_heap_base(),
      });
    } catch (error) {
      postMessage({ type: "error", message: error.stack || error.message });
    }
  } else if (message.type === "start") {
    pid = message.pid;
    launchArgument = message.launchArgument || "";
    launchGeneration++;
    const result = instance.exports._start();
    if (result !== pid) {
      postMessage({ type: "error", message: `remote process start returned ${result}, expected ${pid}` });
      return;
    }
    postMessage({ type: "started", pid });
    scheduleStep();
  } else if (message.type === "launchArgument") {
    launchArgument = message.value || "";
    launchGeneration++;
    try {
      instance.exports.ewok_launch_argument_changed?.();
    } catch (error) {
      postMessage({ type: "error", message: error.stack || error.message });
    }
  } else if (message.type === "step" && instance && pid >= 0) {
    instance.exports.ewok_step();
  }
});
