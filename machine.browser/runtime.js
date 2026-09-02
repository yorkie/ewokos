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

function splitLaunchCommand(commandLine) {
  const trimmed = commandLine.trim();
  const separator = trimmed.search(/\s/);
  if (separator < 0) return { executable: trimmed, argument: "" };
  const executable = trimmed.slice(0, separator);
  let argument = trimmed.slice(separator).trim();
  if (argument.length >= 2 &&
      ((argument[0] === '"' && argument.at(-1) === '"') ||
       (argument[0] === "'" && argument.at(-1) === "'")))
    argument = argument.slice(1, -1);
  return { executable, argument };
}

class EwokHost {
  constructor(memory, output, rootfs, persistRootfs) {
    this.memory = memory;
    this.output = output;
    this.rootfs = rootfs;
    this.persistRootfs = persistRootfs;
    this.persistTimer = null;
    this.decoder = new TextDecoder("utf-8");
    this.encoder = new TextEncoder();
    this.ready = false;
    this.ttyInput = [];
    this.ttyMirrors = new Set();
    this.keyInput = [];
    this.mouseInput = [];
    this.netInput = [];
    this.netTxCount = 0;
    this.netRxCount = 0;
    this.framebuffer = document.querySelector("#framebuffer");
    this.framebufferWidth = 0;
    this.framebufferHeight = 0;
    this.framebufferSource = document.createElement("canvas");
    this.framebufferSourceContext = this.framebufferSource.getContext("2d");
    this.framebufferPixels = null;
    this.requestInputPump = null;
    this.inputStatus = document.querySelector("#input-status");
    this.terminalForm = document.querySelector("#terminal-form");
    this.terminalInput = document.querySelector("#terminal-input");
    this.keyEventCount = 0;
    this.mouseEventCount = 0;
    this.audioSampleCount = 0;
    this.audioContext = null;
    this.audioCursor = 0;
    const netRelay = new URLSearchParams(location.search).get("netRelay");
    this.netSocket = null;
    if (netRelay) {
      this.netSocket = new WebSocket(netRelay);
      this.netSocket.binaryType = "arraybuffer";
      this.netSocket.addEventListener("message", (event) => {
        if (!(event.data instanceof ArrayBuffer)) return;
        this.netInput.push(new Uint8Array(event.data));
        this.netRxCount++;
        this.updateInputStatus();
      });
    }
    this.framebufferContext = this.framebuffer.getContext("2d");
    this.framebufferResizeObserver = typeof ResizeObserver === "undefined" ? null :
      new ResizeObserver(() => this.presentFramebuffer());
    this.framebufferResizeObserver?.observe(this.framebuffer);
    this.framebuffer.addEventListener("keydown", (event) => {
      const special = {
        Enter: 10,
        Backspace: 8,
        Tab: 9,
        Escape: 27,
        ArrowRight: 4,
        ArrowUp: 5,
        ArrowLeft: 19,
        ArrowDown: 24,
        Home: 0xf0,
        End: 0xf1,
      };
      const code = event.key.length === 1 ? event.key.charCodeAt(0) : special[event.key];
      if (code !== undefined && code <= 0xff) {
        this.keyInput.push(code);
        this.keyEventCount++;
        this.updateInputStatus();
        this.requestInputPump?.("key");
        event.preventDefault();
      }
    });
    const queueMouse = (event, state, button = 0) => {
      const rect = this.framebuffer.getBoundingClientRect();
      const x = Math.max(0, Math.min(this.framebufferWidth - 1,
        Math.round((event.clientX - rect.left) * this.framebufferWidth / rect.width)));
      const y = Math.max(0, Math.min(this.framebufferHeight - 1,
        Math.round((event.clientY - rect.top) * this.framebufferHeight / rect.height)));
      const bytes = new Uint8Array(8);
      const view = new DataView(bytes.buffer);
      view.setUint8(0, 2);
      view.setUint8(1, state);
      view.setUint8(2, button);
      view.setInt16(4, x, true);
      view.setInt16(6, y, true);
      /* Pointer-move events can arrive much faster than the guest consumes
       * them. Keep only the newest unconsumed move so the cursor tracks the
       * hand instead of replaying an increasingly stale path. Never coalesce
       * across button transitions. */
      if (state === 1 && this.mouseInput.length >= 8 &&
          this.mouseInput[this.mouseInput.length - 7] === 1) {
        this.mouseInput.splice(this.mouseInput.length - 8, 8);
      }
      this.mouseInput.push(...bytes);
      this.mouseEventCount++;
      this.updateInputStatus();
      this.requestInputPump?.(state === 1 ? "mouse" : "mouse-button");
    };
    this.framebuffer.addEventListener("mousemove", (event) => queueMouse(event, 1));
    this.framebuffer.addEventListener("mousedown", (event) => {
      this.audioContext?.resume();
      this.framebuffer.focus();
      queueMouse(event, 3, event.button === 2 ? 3 : 1);
    });
    this.framebuffer.addEventListener("mouseup", (event) =>
      queueMouse(event, 4, event.button === 2 ? 3 : 1));
    this.framebuffer.addEventListener("wheel", (event) => {
      const vertical = Math.abs(event.deltaY) >= Math.abs(event.deltaX);
      const delta = vertical ? event.deltaY : event.deltaX;
      if (delta === 0) return;
      const button = vertical ? (delta < 0 ? 4 : 5) : (delta < 0 ? 6 : 7);
      queueMouse(event, 1, button);
      event.preventDefault();
    }, { passive: false });
    this.framebuffer.addEventListener("contextmenu", (event) => event.preventDefault());
    this.terminalForm?.addEventListener("submit", (event) => {
      event.preventDefault();
      const command = this.terminalInput.value;
      this.terminalInput.value = "";
      this.queueTerminalText(`${command}\n`);
    });
  }

  setStatus(text, ready = false) {
    const status = document.querySelector("#status");
    status.textContent = text;
    if (ready) status.dataset.state = "ready";
  }

  log(text) {
    this.output.textContent += text;
    this.output.scrollTop = this.output.scrollHeight;
  }

  requestFrame(callback) {
    return requestAnimationFrame(callback);
  }

  installRuntime({ instance, memory, processes, spawn, framebufferProcess }) {
    const guiPanel = document.querySelector("#gui-panel");
    let resizeFrame = 0;
    let guestWidth = this.framebufferWidth;
    let guestHeight = this.framebufferHeight;
    const resizeGuest = () => {
      resizeFrame = 0;
      const width = Math.max(320, Math.min(2560, Math.floor(guiPanel.clientWidth)));
      const height = Math.max(240, Math.min(1600, Math.floor(guiPanel.clientHeight)));
      if (width === guestWidth && height === guestHeight) return;
      const resize = framebufferProcess?.instance.exports.ewok_display_resize;
      if (!resize || resize(width, height) !== 0) {
        this.log(`display resize failed: ${width}x${height}\n`);
        return;
      }
      guestWidth = width;
      guestHeight = height;
    };
    const queueGuestResize = () => {
      if (resizeFrame === 0) resizeFrame = requestAnimationFrame(resizeGuest);
    };
    const guestResizeObserver = typeof ResizeObserver === "undefined" ? null :
      new ResizeObserver(queueGuestResize);
    guestResizeObserver?.observe(guiPanel);
    queueGuestResize();

    let terminalStarted = false;
    const startTerminal = () => {
      if (terminalStarted) return;
      terminalStarted = spawn("/bin/shell") >= 0;
    };
    document.addEventListener("ewok-view-change", (event) => {
      if (event.detail?.view === "terminal") startTerminal();
    });
    if (document.querySelector("#terminal-tab")?.getAttribute("aria-selected") === "true")
      startTerminal();
    window.ewokos = {
      instance,
      memory,
      host: this,
      processes,
      spawn,
      resizeObserver: guestResizeObserver,
      executionMode: "main-thread",
    };
  }

  queueTerminalText(text) {
    this.ttyInput.push(...this.encoder.encode(text));
    this.updateInputStatus();
    this.requestInputPump?.("tty");
  }

  consoleWrite(ptr, len) {
    const bytes = new Uint8Array(this.memory.buffer, ptr, len);
    const text = this.decoder.decode(bytes.slice()).replace(/\x1b\[[0-9;]*[A-Za-z]/g, "");
    this.output.textContent += text;
    this.output.scrollTop = this.output.scrollHeight;
  }

  blockRead(sector, ptr, count) {
    const start = sector * 512;
    const size = count * 512;
    if (!this.rootfs || start + size > this.rootfs.byteLength) return -1;
    new Uint8Array(this.memory.buffer, ptr, size).set(
      this.rootfs.subarray(start, start + size),
    );
    return 0;
  }

  blockWrite(sector, ptr, count) {
    const start = sector * 512;
    const size = count * 512;
    if (!this.rootfs || start + size > this.rootfs.byteLength) return -1;
    this.rootfs.set(new Uint8Array(this.memory.buffer, ptr, size), start);
    this.rootfsDirty = true;
    clearTimeout(this.persistTimer);
    this.persistTimer = setTimeout(() => {
      this.persistRootfs?.(this.rootfs.slice());
      this.rootfsDirty = false;
    }, 250);
    return 0;
  }

  ttyWrite(ptr, len) {
    this.consoleWrite(ptr, len);
    for (const mirror of this.ttyMirrors) {
      try {
        mirror(ptr, len);
      } catch {
        this.ttyMirrors.delete(mirror);
      }
    }
    return len;
  }

  ttyInject(ptr, len) {
    if (len <= 0) return 0;
    this.ttyInput.push(...new Uint8Array(this.memory.buffer, ptr, len));
    this.updateInputStatus();
    this.requestInputPump?.("tty");
    return len;
  }

  ttyRead(ptr, len) {
    const count = Math.min(len, this.ttyInput.length);
    if (count === 0) return 0;
    new Uint8Array(this.memory.buffer, ptr, count).set(
      this.ttyInput.splice(0, count),
    );
    this.updateInputStatus();
    return count;
  }

  framebufferConfigure(width, height) {
    if (width <= 0 || height <= 0) return -1;
    this.framebufferWidth = width;
    this.framebufferHeight = height;
    this.framebufferSource.width = width;
    this.framebufferSource.height = height;
    /* Establish the source aspect ratio before CSS lays out the visible
     * canvas. Its backing store is resized to CSS pixels in presentFramebuffer. */
    this.framebuffer.width = width;
    this.framebuffer.height = height;
    this.framebuffer.hidden = false;
    this.presentFramebuffer();
    return 0;
  }

  presentFramebuffer() {
    if (this.framebufferWidth <= 0 || this.framebufferHeight <= 0 ||
        this.framebuffer.hidden) return;
    const cssWidth = this.framebuffer.clientWidth;
    const cssHeight = this.framebuffer.clientHeight;
    if (cssWidth <= 0 || cssHeight <= 0) return;
    const scale = Math.min(window.devicePixelRatio || 1, 2);
    const targetWidth = Math.max(1, Math.round(cssWidth * scale));
    const targetHeight = Math.max(1, Math.round(cssHeight * scale));
    if (this.framebuffer.width !== targetWidth ||
        this.framebuffer.height !== targetHeight) {
      this.framebuffer.width = targetWidth;
      this.framebuffer.height = targetHeight;
      this.framebufferContext = this.framebuffer.getContext("2d");
    }
    this.framebufferContext.imageSmoothingEnabled = true;
    this.framebufferContext.imageSmoothingQuality = "high";
    this.framebufferContext.clearRect(0, 0, targetWidth, targetHeight);
    this.framebufferContext.drawImage(this.framebufferSource,
      0, 0, this.framebufferWidth, this.framebufferHeight,
      0, 0, targetWidth, targetHeight);
  }

  updateInputStatus() {
    this.inputStatus.textContent =
      `Keyboard ${this.keyEventCount} · pointer ${this.mouseEventCount} · audio ${this.audioSampleCount}`;
    this.inputStatus.textContent += ` · tty ${this.ttyInput.length} · net ${this.netTxCount}/${this.netRxCount}`;
  }

  netWrite(ptr, len) {
    if (len < 14 || len > 65535) return -1;
    const frame = new Uint8Array(this.memory.buffer, ptr, len).slice();
    if (this.netSocket?.readyState === WebSocket.OPEN)
      this.netSocket.send(frame);
    this.netTxCount++;
    this.updateInputStatus();
    return len;
  }

  netRead(ptr, len) {
    const frame = this.netInput[0];
    if (!frame) return 0;
    if (frame.byteLength > len) return -1;
    this.netInput.shift();
    new Uint8Array(this.memory.buffer, ptr, frame.byteLength).set(frame);
    return frame.byteLength;
  }

  audioWrite(ptr, size, rate, channels, bitDepth) {
    if (size <= 0 || rate <= 0 || channels <= 0 || channels > 8) return -1;
    const bytesPerSample = Math.abs(bitDepth) / 8;
    if (![1, 2, 4].includes(bytesPerSample) || size % (bytesPerSample * channels)) return -1;
    const frames = size / bytesPerSample / channels;
    this.audioContext ??= new AudioContext({ sampleRate: rate });
    const audio = this.audioContext.createBuffer(channels, frames, rate);
    const view = new DataView(this.memory.buffer, ptr, size);
    for (let channel = 0; channel < channels; channel++) {
      const output = audio.getChannelData(channel);
      for (let frame = 0; frame < frames; frame++) {
        const offset = (frame * channels + channel) * bytesPerSample;
        if (bitDepth === 16) output[frame] = view.getInt16(offset, true) / 32768;
        else if (bitDepth === 32) output[frame] = view.getInt32(offset, true) / 2147483648;
        else if (bitDepth === -8) output[frame] = (view.getUint8(offset) - 128) / 128;
        else output[frame] = view.getInt8(offset) / 128;
      }
    }
    const source = this.audioContext.createBufferSource();
    source.buffer = audio;
    source.connect(this.audioContext.destination);
    const now = this.audioContext.currentTime;
    this.audioCursor = Math.max(now, this.audioCursor);
    source.start(this.audioCursor);
    this.audioCursor += frames / rate;
    this.audioSampleCount += frames;
    this.updateInputStatus();
    return size;
  }

  framebufferFlush(ptr, width, height) {
    if (
      width !== this.framebufferWidth ||
      height !== this.framebufferHeight ||
      ptr + width * height * 4 > this.memory.buffer.byteLength
    ) {
      return -1;
    }
    const source = new Uint8Array(this.memory.buffer, ptr, width * height * 4);
    if (this.framebufferPixels?.length !== source.length)
      this.framebufferPixels = new Uint8ClampedArray(source.length);
    const rgba = this.framebufferPixels;
    for (let offset = 0; offset < source.length; offset += 4) {
      rgba[offset] = source[offset + 2];
      rgba[offset + 1] = source[offset + 1];
      rgba[offset + 2] = source[offset];
      rgba[offset + 3] = source[offset + 3];
    }
    this.framebufferSourceContext.putImageData(
      new ImageData(rgba, width, height),
      0,
      0,
    );
    this.presentFramebuffer();
    return 0;
  }

  inputRead(queue, ptr, len) {
    const count = Math.min(len, queue.length);
    if (count === 0) return 0;
    new Uint8Array(this.memory.buffer, ptr, count).set(queue.splice(0, count));
    return count;
  }

  imports() {
    return {
      env: {
        memory: this.memory,
        console_write: (ptr, len) => this.consoleWrite(ptr, len),
        wasm_host_now_usec: () => BigInt(Math.floor(performance.now() * 1000)),
        wasm_host_wait: () => {},
        wasm_host_halt: () => {
          throw new WebAssembly.RuntimeError("EwokOS halted");
        },
        /* Userspace sdfsd owns the filesystem.  The wasm kernel boots from
         * the module manifest and must not parse/load native ELF content. */
        wasm_host_block_read: () => -1,
        wasm_host_block_write: (sector, ptr, count) =>
          this.blockWrite(sector, ptr, count),
        wasm_host_block_flush: () => 0,
        wasm_host_ready: () => {
          this.ready = true;
        },
      },
    };
  }
}

async function loadOptionalRootfs(url) {
  const response = await fetch(url, { cache: "no-store" });
  if (!response.ok) return null;
  return new Uint8Array(await response.arrayBuffer());
}

function openRootfsDatabase() {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open("ewokos-wasm", 1);
    request.onupgradeneeded = () =>
      request.result.createObjectStore("images");
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
  });
}

function databaseRequest(request) {
  return new Promise((resolve, reject) => {
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
  });
}

async function loadPersistentRootfs(url) {
  const bundled = await loadOptionalRootfs(url);
  if (!bundled || !globalThis.indexedDB)
    return { image: bundled, persist: null };
  try {
    const database = await openRootfsDatabase();
    const transaction = database.transaction("images", "readonly");
    const saved = await databaseRequest(transaction.objectStore("images").get(url));
    const image = saved instanceof ArrayBuffer && saved.byteLength === bundled.byteLength
      ? new Uint8Array(saved) : bundled;
    const persist = (bytes) => {
      const write = database.transaction("images", "readwrite");
      write.objectStore("images").put(bytes.buffer, url);
    };
    return { image, persist };
  } catch (error) {
    console.warn("EwokOS rootfs persistence unavailable", error);
    return { image: bundled, persist: null };
  }
}

async function bootEwokRuntime(host, memory) {
  const response = await fetch("kernel/kernel.wasm", { cache: "no-store" });
  const result = await WebAssembly.instantiateStreaming(response, host.imports());

  result.instance.exports._start();
  const mmuResult = result.instance.exports.wasm_mmu_self_test();
  if (!host.ready || mmuResult !== 0) {
    throw new Error(`kernel=${host.ready}, software MMU=${mmuResult}`);
  }
  const manifestResponse = await fetch("user/modules.json", { cache: "no-store" });
  const manifest = await manifestResponse.json();
  const remoteHandles = host.prepareProcessWorkers ?
    await host.prepareProcessWorkers(
      manifest.modules.filter((descriptor) => descriptor.worker === true),
      memory,
    ) : new Map();
  const compiledModules = new Map(
    await Promise.all(
      manifest.modules.filter((descriptor) => !remoteHandles.has(descriptor.path))
        .map(async (descriptor) => {
        const moduleResponse = await fetch(descriptor.url, { cache: "no-store" });
        if (!moduleResponse.ok) {
          throw new Error(`could not load ${descriptor.url}`);
        }
        return [
          descriptor.url,
          await WebAssembly.compile(await moduleResponse.arrayBuffer()),
        ];
      }),
    ),
  );
  const processes = [];
  const moduleRanges = [];
  let dispatchDepth = 0;
  const kernelTraceCallers = new Int32Array(64);
  const kernelTraceCodes = new Int32Array(64);
  const kernelTraceTargets = new Int32Array(64);
  let kernelTraceCount = 0;
  let kernelTraceCursor = 0;
  const recordKernelTrace = (caller, code, target) => {
    kernelTraceCallers[kernelTraceCursor] = caller;
    kernelTraceCodes[kernelTraceCursor] = code;
    kernelTraceTargets[kernelTraceCursor] = target;
    kernelTraceCursor = (kernelTraceCursor + 1) % kernelTraceCallers.length;
    kernelTraceCount = Math.min(kernelTraceCount + 1, kernelTraceCallers.length);
  };
  const recentKernelTrace = (limit) => {
    const count = Math.min(limit, kernelTraceCount);
    const start = (kernelTraceCursor - count + kernelTraceCallers.length) %
      kernelTraceCallers.length;
    return Array.from(
      { length: count },
      (_, offset) => {
        const index = (start + offset) % kernelTraceCallers.length;
        return `${kernelTraceCallers[index]}:${kernelTraceCodes[index]}>${kernelTraceTargets[index]}`;
      },
    );
  };
  const terminalDispatchSyscalls = new Set([11, 17, 43, 50]);
  const syscallFor = (callerPid, code, arg0, arg1, arg2) => {
    if (result.instance.exports.wasm_kernel_activate(callerPid) !== 0) return -1;
    const syscallResult = result.instance.exports.wasm_kernel_syscall(
      code,
      arg0,
      arg1,
      arg2,
    );
    const currentPid = result.instance.exports.wasm_kernel_current_pid();
    recordKernelTrace(callerPid, code, currentPid);
    if (currentPid === callerPid) {
      return syscallResult;
    }
    if (
      dispatchDepth > 0 &&
      terminalDispatchSyscalls.has(code) &&
      currentPid !== callerPid
    ) {
      return result.instance.exports.wasm_kernel_result(callerPid);
    }

    let dispatches = 0;
    const dispatchTrace = [];
    while (result.instance.exports.wasm_kernel_current_pid() !== callerPid) {
      if (!result.instance.exports.wasm_kernel_current_dispatchable()) {
        if (result.instance.exports.wasm_kernel_runnable(callerPid)) {
          result.instance.exports.wasm_kernel_activate(callerPid);
          continue;
        }
      }
      if (++dispatches > 1024) {
        throw new Error(
          `kernel dispatch did not return to pid ${callerPid}: ${dispatchTrace.slice(-8).join(" -> ")}; syscalls=${recentKernelTrace(16).join(",")}`,
        );
      }
      const targetPid = result.instance.exports.wasm_kernel_current_pid();
      const ownerPid = result.instance.exports.wasm_kernel_current_owner_pid();
      const target = processes.find((process) => process.pid === ownerPid);
      const dispatch = target?.instance?.exports.ewok_dispatch;
      if (!dispatch) {
        throw new Error(`pid ${targetPid} has no wasm context dispatcher`);
      }
      const entry = result.instance.exports.wasm_kernel_current_entry();
      const arg0 = result.instance.exports.wasm_kernel_current_arg(0);
      const arg1 = result.instance.exports.wasm_kernel_current_arg(1);
      const previousContextPid = target.context.pid;
      dispatchTrace.push(`${targetPid}/${ownerPid}@${entry}`);
      target.context.pid = targetPid;
      dispatchDepth++;
      try {
        if (dispatch(entry, arg0, arg1) !== 0) {
          throw new Error(`pid ${targetPid} rejected entry ${entry}`);
        }
      } finally {
        dispatchDepth--;
        target.context.pid = previousContextPid;
      }
    }
    return result.instance.exports.wasm_kernel_result(callerPid);
  };
  let spawnDescriptor;
  const spawnPath = (requestedPath) => {
    const { executable, argument } = splitLaunchCommand(requestedPath);
    const candidates = executable.startsWith("/") ? [executable] :
      [`/bin/${executable}`, `/sbin/${executable}`, executable];
    const descriptor = manifest.modules.find((item) => candidates.includes(item.path));
    if (!descriptor) return -1;
    const existing = processes.find((process) => process.path === descriptor.path);
    if (existing) {
      existing.context.launchArgument = argument;
      existing.context.launchGeneration++;
      existing.remote?.setLaunchArgument(argument);
      return existing.pid;
    }
    try {
      const pid = spawnDescriptor(descriptor, argument).pid;
      host.setStatus(
        `EwokOS kernel + ${processes.length} wasm processes ready · ${processes.filter((process) => process.hostPoll).length} host IRQ sources · software MMU passed`,
      );
      return pid;
    } catch (error) {
      host.log(`wasm loader: ${error.message}\n`);
      return -1;
    }
  };
  const spawnCommand = (ptr, length) => {
    const commandLine = host.decoder.decode(
      new Uint8Array(memory.buffer, ptr, length).slice(),
    );
    return spawnPath(commandLine);
  };
  spawnDescriptor = (descriptor, launchArgument = "") => {
    const url = descriptor.url;
    let pid = -1;
    const context = { pid: -1, launchArgument, launchGeneration: 1 };
    const remote = remoteHandles.get(descriptor.path);
    const userInstance = remote ? null : new WebAssembly.Instance(compiledModules.get(url), {
      env: {
        ...wasmMathImports,
        memory,
        ewok_syscall: (code, arg0, arg1, arg2) =>
          syscallFor(context.pid, code, arg0, arg1, arg2),
        wasm_host_block_read: (sector, ptr, count) =>
          host.blockRead(sector, ptr, count),
        wasm_host_block_write: (sector, ptr, count) =>
          host.blockWrite(sector, ptr, count),
        wasm_host_block_flush: () => 0,
        wasm_host_tty_write: (ptr, len) => host.ttyWrite(ptr, len),
        wasm_host_tty_read: (ptr, len) => host.ttyRead(ptr, len),
        wasm_host_tty_available: () => host.ttyInput.length,
        wasm_host_tty_input: (ptr, len) => host.ttyInject(ptr, len),
        wasm_host_spawn_command: (ptr, len) => spawnCommand(ptr, len),
        wasm_host_launch_argument: (ptr, capacity) => {
          if (capacity <= 0) return 0;
          const bytes = host.encoder.encode(context.launchArgument || "");
          const copied = Math.min(bytes.length, capacity - 1);
          const target = new Uint8Array(memory.buffer, ptr, capacity);
          target.set(bytes.subarray(0, copied));
          target[copied] = 0;
          return copied;
        },
        wasm_host_launch_generation: () => context.launchGeneration,
        wasm_host_framebuffer_configure: (width, height) =>
          host.framebufferConfigure(width, height),
        wasm_host_framebuffer_flush: (ptr, width, height) =>
          host.framebufferFlush(ptr, width, height),
        wasm_host_key_read: (ptr, len) => host.inputRead(host.keyInput, ptr, len),
        wasm_host_key_available: () => host.keyInput.length,
        wasm_host_mouse_read: (ptr, len) =>
          host.inputRead(host.mouseInput, ptr, len),
        wasm_host_mouse_available: () => host.mouseInput.length,
        wasm_host_audio_write: (ptr, size, rate, channels, bitDepth) =>
          host.audioWrite(ptr, size, rate, channels, bitDepth),
        wasm_host_unix_time_sec: () => Math.floor(Date.now() / 1000),
        wasm_host_net_read: (ptr, len) => host.netRead(ptr, len),
        wasm_host_net_write: (ptr, len) => host.netWrite(ptr, len),
        wasm_host_net_available: () => host.netInput.length,
      },
    });
    const exports = userInstance?.exports;
    const command = remote?.metadata.command ?? exports.ewok_init_command();
    const commandSize = remote?.metadata.commandSize ?? exports.ewok_init_command_size();
    const moduleBase = remote?.metadata.moduleBase ?? exports.ewok_module_base();
    const moduleSize = remote?.metadata.moduleSize ?? exports.ewok_module_size();
    const heapBase = remote?.metadata.heapBase ?? exports.ewok_heap_base();
    const moduleEnd = moduleBase + moduleSize;
    const physicalLimit = result.instance.exports.wasm_kernel_physical_limit();
    const moduleLimit = result.instance.exports.wasm_kernel_module_limit();
    const conflictingRange = moduleRanges.find(
      ({ base, end }) => moduleBase < end && base < moduleEnd,
    );
    if (
      moduleEnd < moduleBase ||
      moduleBase < physicalLimit ||
      moduleEnd > moduleLimit ||
      conflictingRange
    ) {
      throw new Error(
        `${url} has unsafe wasm memory range 0x${moduleBase.toString(16)}..0x${moduleEnd.toString(16)}` +
        (conflictingRange ? ` overlapping ${conflictingRange.url} at 0x${conflictingRange.base.toString(16)}..0x${conflictingRange.end.toString(16)}` : ""),
      );
    }
    moduleRanges.push({ base: moduleBase, end: moduleEnd, url });
    pid = result.instance.exports.wasm_kernel_spawn(
      command,
      commandSize,
      moduleBase,
      moduleSize,
      heapBase,
    );
    if (
      descriptor.system &&
      result.instance.exports.wasm_kernel_set_system_process(pid) !== 0
    ) {
      throw new Error(`${url} could not acquire kernel-service identity`);
    }
    context.pid = pid;
    /*
     * Native EwokOS runs core concurrently with process startup.  core drains
     * KEV_PROC_CREATED and forwards VFS_PROC_CLONE before the child reaches
     * userspace, which materializes its fd table in vfsd.  Browser wasm
     * modules share one JS thread, so reproduce that ordering explicitly
     * before calling the new module's _start entry point.
     */
    const coreProcess = processes.find(
      (process) => process.path === "/sbin/core",
    );
    if (coreProcess) {
      let coreSteps = 0;
      while (coreSteps++ < 64 && coreProcess.instance.exports.ewok_step() === 0) {
        // Keep draining queued kernel events until SYS_GET_KEVENT reports empty.
      }
    }
    if (remote && pid >= 0) {
      remote.bindSyscall((code, arg0, arg1, arg2) => {
        if (code === WASM_HOSTCALL_SPAWN_COMMAND)
          return spawnCommand(arg0, arg1);
        return syscallFor(pid, code, arg0, arg1, arg2);
      });
      remote.start(pid, context.launchArgument);
    }
    const startResult = pid < 0 ? -1 : (remote ? pid : userInstance.exports._start());
    if (pid < 0 || startResult !== pid) {
      const stage = userInstance?.exports.ewok_start_stage?.();
      const serviceStage = userInstance?.exports.ewok_sdfsd_stage?.();
      const probeResult = userInstance?.exports.ewok_rootfs_probe_result?.();
      const programResult = userInstance?.exports.ewok_program_result?.();
      const displayStage = userInstance?.exports.ewok_display_probe_stage?.();
      throw new Error(
        `${url} failed to start (pid=${pid}, result=${startResult}, stage=${stage ?? "n/a"}/${serviceStage ?? "n/a"}, probe=${probeResult ?? "n/a"}, program=${programResult ?? "n/a"}, display=${displayStage ?? "n/a"})`,
      );
    }
    const process = {
      path: descriptor.path,
      url,
      pid,
      instance: userInstance,
      remote,
      moduleBase,
      moduleSize,
      context,
      hostPoll: descriptor.hostPoll === true,
      pollIntervalMs: Math.max(0, descriptor.pollIntervalMs ?? 50),
      nextPollAt: 0,
    };
    processes.push(process);
    if (typeof userInstance?.exports.ewok_xterm_write === "function")
      host.ttyMirrors.add(userInstance.exports.ewok_xterm_write);
    return process;
  };
  for (const descriptor of manifest.modules) {
    if (descriptor.autostart === false) continue;
    spawnDescriptor(descriptor);
  }
  for (const process of processes) process.instance?.exports.ewok_step();
  const runProcess = (process, force = false) => {
    if (process?.remote) return;
    if (!process || (!force && !process.hostPoll &&
        !result.instance.exports.wasm_kernel_runnable(process.pid))) return;
    try {
      process.instance.exports.ewok_step();
    } catch (error) {
      host.log(`wasm scheduler: ${process.path}: ${error.message}\n`);
      process.hostPoll = false;
    }
  };
  const runProcessPath = (path, force = false) =>
    runProcess(processes.find((process) => process.path === path), force);
  const runXClients = () => {
    for (const process of processes) {
      if (process.path === "/bin/x/xlauncher" || process.path.startsWith("/apps/"))
        runProcess(process);
    }
  };
  const runProcessPass = (now) => {
    for (const process of processes) {
      if (now < process.nextPollAt) continue;
      process.nextPollAt = now + process.pollIntervalMs;
      runProcess(process);
    }
  };
  let inputPumpQueued = false;
  const pendingInputKinds = new Set();
  host.requestInputPump = (kind) => {
    pendingInputKinds.add(kind);
    if (inputPumpQueued) return;
    inputPumpQueued = true;
    queueMicrotask(() => {
      inputPumpQueued = false;
      const kinds = new Set(pendingInputKinds);
      pendingInputKinds.clear();
      if (kinds.has("mouse") || kinds.has("mouse-button")) {
        runProcessPath("/drivers/wasm/moused", true);
        runProcessPath("/sbin/x/xmouse", true);
        /* Cursor movement is handled synchronously by xserver. Applications
         * only need immediate delivery for button transitions; their normal
         * cadence remains the fallback for hover events. */
        if (kinds.has("mouse-button"))
          runXClients();
      }
      if (kinds.has("key")) {
        runProcessPath("/drivers/wasm/keybd", true);
        runProcessPath("/sbin/x/xim_none", true);
        runXClients();
      }
      if (kinds.has("tty") || kinds.has("key")) {
        runProcessPath("/bin/shell", true);
        runProcessPath("/apps/xterm/xterm", true);
      }
      if (kinds.has("mouse-button") || kinds.has("key") || kinds.has("tty")) {
        runProcessPath("/drivers/xserverd", true);
      }
    });
  };
  let lastFrame = performance.now();
  const scheduleFrame = (now) => {
    const elapsedUsec = Math.max(0, Math.floor((now - lastFrame) * 1000));
    lastFrame = now;
    result.instance.exports.wasm_kernel_tick(elapsedUsec);
    runProcessPass(now);
    host.frameRequest = host.requestFrame(scheduleFrame);
  };
  host.frameRequest = host.requestFrame(scheduleFrame);
  host.setStatus(
    `EwokOS kernel + ${processes.length} wasm processes ready · ${processes.filter((process) => process.hostPoll).length} host IRQ sources · software MMU passed`,
    true,
  );
  const framebufferProcess = processes.find(
    (process) => process.path === "/drivers/wasm/fbdisplayd",
  );
  host.installRuntime({
    instance: result.instance,
    memory,
    processes,
    spawn: spawnPath,
    framebufferProcess,
  });
}

async function bootEwokOSLocal() {
  const output = document.querySelector("#console");
  const memory = new WebAssembly.Memory({
    initial: 14336,
    maximum: 14336,
    shared: true,
  });
  const rootfsState = await loadPersistentRootfs("rootfs.img")
    .catch(() => ({ image: null, persist: null }));
  const host = new EwokHost(memory, output, rootfsState.image, rootfsState.persist);
  return bootEwokRuntime(host, memory);
}

async function bootEwokOS() {
  if (!crossOriginIsolated || typeof SharedArrayBuffer === "undefined") {
    throw new Error(
      "Wasm workers require cross-origin isolation; start this target with `make -C machine.browser serve`",
    );
  }

  const framebuffer = document.querySelector("#framebuffer");
  const guiPanel = document.querySelector("#gui-panel");
  const output = document.querySelector("#console");
  const status = document.querySelector("#status");
  const inputStatus = document.querySelector("#input-status");
  const terminalForm = document.querySelector("#terminal-form");
  const terminalInput = document.querySelector("#terminal-input");
  const workerUrl = new URL("runtime-worker.js", location.href);
  const netRelay = new URLSearchParams(location.search).get("netRelay");
  if (netRelay) workerUrl.searchParams.set("netRelay", netRelay);
  const worker = new Worker(workerUrl, { name: "ewokos-kernel" });
  const compositorWorker = new Worker("compositor-worker.js", {
    name: "ewokos-compositor",
  });
  let framebufferWidth = 640;
  let framebufferHeight = 480;
  let audioContext = null;
  let audioCursor = 0;
  let pendingMove = null;
  let moveFrame = 0;
  let pendingBitmap = null;
  let bitmapFrame = 0;
  let terminalStarted = false;

  const updateCanvasSize = () => {
    const scale = Math.min(window.devicePixelRatio || 1, 2);
    const width = Math.max(1, Math.round(framebuffer.clientWidth * scale));
    const height = Math.max(1, Math.round(framebuffer.clientHeight * scale));
    if (framebuffer.width !== width) framebuffer.width = width;
    if (framebuffer.height !== height) framebuffer.height = height;
  };
  const presentBitmap = (bitmap) => {
    updateCanvasSize();
    const context = framebuffer.getContext("2d");
    context.imageSmoothingEnabled = true;
    context.imageSmoothingQuality = "high";
    context.clearRect(0, 0, framebuffer.width, framebuffer.height);
    context.drawImage(bitmap, 0, 0, framebuffer.width, framebuffer.height);
    bitmap.close();
  };
  /* The compositor may finish several complete guest frames between two
   * browser paints (focus changes commonly produce an unfocused frame and a
   * focused frame back-to-back). Presenting every intermediate bitmap lets
   * those otherwise valid frames flash in sequence when the main thread is
   * busy. Keep only the newest bitmap for the next vsync, and release the
   * superseded GPU resource immediately. */
  const queueBitmap = (bitmap) => {
    if (pendingBitmap) pendingBitmap.close();
    pendingBitmap = bitmap;
    if (bitmapFrame) return;
    bitmapFrame = requestAnimationFrame(() => {
      bitmapFrame = 0;
      const latest = pendingBitmap;
      pendingBitmap = null;
      if (latest) presentBitmap(latest);
    });
  };
  const playAudio = ({ samples, rate, channels, bitDepth }) => {
    const bytesPerSample = Math.abs(bitDepth) / 8;
    if (![1, 2, 4].includes(bytesPerSample) || channels <= 0) return;
    const frames = samples.byteLength / bytesPerSample / channels;
    audioContext ??= new AudioContext({ sampleRate: rate });
    const audio = audioContext.createBuffer(channels, frames, rate);
    const view = new DataView(samples);
    for (let channel = 0; channel < channels; channel++) {
      const channelData = audio.getChannelData(channel);
      for (let frame = 0; frame < frames; frame++) {
        const offset = (frame * channels + channel) * bytesPerSample;
        if (bitDepth === 16) channelData[frame] = view.getInt16(offset, true) / 32768;
        else if (bitDepth === 32) channelData[frame] = view.getInt32(offset, true) / 2147483648;
        else if (bitDepth === -8) channelData[frame] = (view.getUint8(offset) - 128) / 128;
        else channelData[frame] = view.getInt8(offset) / 128;
      }
    }
    const source = audioContext.createBufferSource();
    source.buffer = audio;
    source.connect(audioContext.destination);
    audioCursor = Math.max(audioContext.currentTime, audioCursor);
    source.start(audioCursor);
    audioCursor += frames / rate;
  };

  const ready = new Promise((resolve, reject) => {
    compositorWorker.addEventListener("message", (event) => {
      if (event.data.type === "framebuffer") queueBitmap(event.data.bitmap);
      else if (event.data.type === "framebuffer-buffer")
        worker.postMessage(event.data, [event.data.buffer]);
      else if (event.data.type === "compositor-mode")
        status.dataset.compositor = event.data.mode;
    });
    compositorWorker.addEventListener("error", (event) =>
      reject(event.error || new Error(event.message)));
    worker.addEventListener("message", (event) => {
      const message = event.data;
      if (message.type === "console") {
        output.textContent += message.text;
        output.scrollTop = output.scrollHeight;
      } else if (message.type === "status") {
        status.textContent = message.text;
        if (message.ready) {
          status.dataset.state = "ready";
          status.dataset.executionMode = "worker-pipeline";
        }
      } else if (message.type === "input-status") {
        inputStatus.textContent = message.text;
      } else if (message.type === "framebuffer-configure") {
        framebufferWidth = message.width;
        framebufferHeight = message.height;
        framebuffer.hidden = false;
        updateCanvasSize();
        compositorWorker.postMessage({
          type: "configure",
          width: message.width,
          height: message.height,
        });
      } else if (message.type === "framebuffer-flush") {
        const transfer = message.pixels instanceof ArrayBuffer ?
          [message.pixels] : [];
        compositorWorker.postMessage({ ...message, type: "flush" }, transfer);
      } else if (message.type === "audio") {
        playAudio(message);
      } else if (message.type === "runtime-ready") {
        compositorWorker.postMessage({ type: "init", memory: message.memory });
        window.ewokos = {
          worker,
          compositorWorker,
          executionMode: "worker-pipeline",
          sharedMemory: message.sharedMemory,
          processCount: message.processCount,
          spawn: (path) => worker.postMessage({ type: "spawn", path }),
        };
        sendResize();
        resolve(window.ewokos);
      } else if (message.type === "fatal") {
        reject(new Error(message.message));
      }
    });
    worker.addEventListener("error", (event) => reject(event.error || new Error(event.message)));
  });

  const sendMouse = (event, state, button = 0) => {
    const rect = framebuffer.getBoundingClientRect();
    const x = Math.max(0, Math.min(framebufferWidth - 1,
      Math.round((event.clientX - rect.left) * framebufferWidth / rect.width)));
    const y = Math.max(0, Math.min(framebufferHeight - 1,
      Math.round((event.clientY - rect.top) * framebufferHeight / rect.height)));
    worker.postMessage({ type: "mouse", state, button, x, y });
  };
  framebuffer.addEventListener("mousemove", (event) => {
    pendingMove = event;
    if (moveFrame !== 0) return;
    moveFrame = requestAnimationFrame(() => {
      moveFrame = 0;
      const latest = pendingMove;
      pendingMove = null;
      if (latest) sendMouse(latest, 1);
    });
  });
  framebuffer.addEventListener("mousedown", (event) => {
    audioContext?.resume();
    framebuffer.focus();
    sendMouse(event, 3, event.button === 2 ? 3 : 1);
  });
  framebuffer.addEventListener("mouseup", (event) =>
    sendMouse(event, 4, event.button === 2 ? 3 : 1));
  framebuffer.addEventListener("wheel", (event) => {
    const vertical = Math.abs(event.deltaY) >= Math.abs(event.deltaX);
    const delta = vertical ? event.deltaY : event.deltaX;
    if (delta === 0) return;
    const button = vertical ? (delta < 0 ? 4 : 5) : (delta < 0 ? 6 : 7);
    sendMouse(event, 1, button);
    event.preventDefault();
  }, { passive: false });
  framebuffer.addEventListener("contextmenu", (event) => event.preventDefault());
  framebuffer.addEventListener("keydown", (event) => {
    const special = {
      Enter: 10, Backspace: 8, Tab: 9, Escape: 27,
      ArrowRight: 4, ArrowUp: 5, ArrowLeft: 19, ArrowDown: 24,
      Home: 0xf0, End: 0xf1,
    };
    const code = event.key.length === 1 ? event.key.charCodeAt(0) : special[event.key];
    if (code !== undefined && code <= 0xff) {
      worker.postMessage({ type: "key", code });
      event.preventDefault();
    }
  });
  terminalForm?.addEventListener("submit", (event) => {
    event.preventDefault();
    worker.postMessage({ type: "tty", text: `${terminalInput.value}\n` });
    terminalInput.value = "";
  });
  document.addEventListener("ewok-view-change", (event) => {
    if (event.detail?.view === "terminal" && !terminalStarted) {
      terminalStarted = true;
      worker.postMessage({ type: "spawn", path: "/bin/shell" });
    }
  });

  let resizeFrame = 0;
  const sendResize = () => {
    resizeFrame = 0;
    worker.postMessage({
      type: "resize",
      width: Math.max(320, Math.min(2560, Math.floor(guiPanel.clientWidth))),
      height: Math.max(240, Math.min(1600, Math.floor(guiPanel.clientHeight))),
    });
  };
  const resizeObserver = new ResizeObserver(() => {
    if (resizeFrame === 0) resizeFrame = requestAnimationFrame(sendResize);
  });
  resizeObserver.observe(guiPanel);
  sendResize();
  return ready;
}
