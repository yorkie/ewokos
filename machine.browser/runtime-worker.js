importScripts("runtime.js");

class WorkerEwokHost {
  constructor(memory, rootfs, persistRootfs, netRelay) {
    this.memory = memory;
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
    this.keyEventCount = 0;
    this.mouseEventCount = 0;
    this.audioSampleCount = 0;
    this.framebufferWidth = 0;
    this.framebufferHeight = 0;
    this.framebufferBuffers = [];
    this.requestInputPump = null;
    this.spawn = null;
    this.framebufferProcess = null;
    this.resizeWidth = 0;
    this.resizeHeight = 0;
    this.netSocket = null;
    this.workerCount = 2;
    if (netRelay) {
      this.netSocket = new WebSocket(netRelay);
      this.netSocket.binaryType = "arraybuffer";
      this.netSocket.addEventListener("message", (event) => {
        if (!(event.data instanceof ArrayBuffer)) return;
        this.netInput.push(new Uint8Array(event.data));
        this.netRxCount++;
        this.updateInputStatus();
        this.requestInputPump?.("net");
      });
    }
  }

  setStatus(text, ready = false) {
    postMessage({ type: "status", text: `${text} · ${this.workerCount} workers`, ready });
  }

  log(text) {
    postMessage({ type: "console", text });
  }

  requestFrame(callback) {
    return setTimeout(() => callback(performance.now()), 16);
  }

  async prepareProcessWorkers(descriptors, memory) {
    const entries = await Promise.all(descriptors.map((descriptor) =>
      new Promise((resolve, reject) => {
        const worker = new Worker("process-worker.js", {
          name: `ewokos:${descriptor.path}`,
        });
        const mailboxBuffer = new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * 8);
        const mailbox = new Int32Array(mailboxBuffer);
        let syscallHandler = null;
        let prepared = false;
        worker.addEventListener("message", (event) => {
          const message = event.data;
          if (message.type === "prepared") {
            prepared = true;
            resolve([descriptor.path, {
              metadata: message,
              bindSyscall(handler) { syscallHandler = handler; },
              start(pid, launchArgument = "") {
                worker.postMessage({ type: "start", pid, launchArgument });
              },
              setLaunchArgument(value) {
                worker.postMessage({ type: "launchArgument", value });
              },
              worker,
            }]);
          } else if (message.type === "syscall") {
            let result = -1;
            try {
              result = syscallHandler?.(
                Atomics.load(mailbox, 1),
                Atomics.load(mailbox, 2),
                Atomics.load(mailbox, 3),
                Atomics.load(mailbox, 4),
              ) ?? -1;
            } catch (error) {
              this.log(`process worker syscall failed (${descriptor.path}): ${error.message}\n`);
            }
            Atomics.store(mailbox, 5, result);
            Atomics.store(mailbox, 0, 2);
            Atomics.notify(mailbox, 0, 1);
          } else if (message.type === "error") {
            const error = new Error(`${descriptor.path}: ${message.message}`);
            if (!prepared) reject(error);
            else this.log(`process worker failed: ${error.message}\n`);
          }
        });
        worker.addEventListener("error", (event) => {
          const error = event.error || new Error(event.message);
          if (!prepared) reject(error);
          else this.log(`process worker failed (${descriptor.path}): ${error.message}\n`);
        });
        worker.postMessage({
          type: "prepare",
          url: descriptor.url,
          memory,
          mailbox: mailboxBuffer,
          pollIntervalMs: descriptor.pollIntervalMs,
        });
      })
    ));
    this.workerCount += entries.length;
    return new Map(entries);
  }

  installRuntime({ processes, spawn, framebufferProcess }) {
    this.spawn = spawn;
    this.framebufferProcess = framebufferProcess;
    postMessage({
      type: "runtime-ready",
      processCount: processes.length,
      sharedMemory: this.memory.buffer instanceof SharedArrayBuffer,
      memory: this.memory,
    });
    this.applyResize();
  }

  handleMessage(message) {
    switch (message.type) {
      case "key":
        this.keyInput.push(message.code);
        this.keyEventCount++;
        this.updateInputStatus();
        this.requestInputPump?.("key");
        break;
      case "mouse": {
        const bytes = new Uint8Array(8);
        const view = new DataView(bytes.buffer);
        view.setUint8(0, 2);
        view.setUint8(1, message.state);
        view.setUint8(2, message.button || 0);
        view.setInt16(4, message.x, true);
        view.setInt16(6, message.y, true);
        if (message.state === 1 && this.mouseInput.length >= 8 &&
            this.mouseInput[this.mouseInput.length - 7] === 1) {
          this.mouseInput.splice(this.mouseInput.length - 8, 8);
        }
        this.mouseInput.push(...bytes);
        this.mouseEventCount++;
        this.updateInputStatus();
        this.requestInputPump?.(message.state === 1 ? "mouse" : "mouse-button");
        break;
      }
      case "tty":
        this.queueTerminalText(message.text);
        break;
      case "spawn":
        this.spawn?.(message.path);
        break;
      case "resize":
        this.resizeWidth = message.width;
        this.resizeHeight = message.height;
        this.applyResize();
        break;
      case "framebuffer-buffer":
        if (message.buffer instanceof ArrayBuffer &&
            this.framebufferBuffers.length < 3)
          this.framebufferBuffers.push(message.buffer);
        break;
      case "audio-resumed":
        break;
    }
  }

  applyResize() {
    if (!this.framebufferProcess || this.resizeWidth <= 0 || this.resizeHeight <= 0)
      return;
    const resize = this.framebufferProcess.instance.exports.ewok_display_resize;
    if (!resize || resize(this.resizeWidth, this.resizeHeight) !== 0)
      this.log(`display resize failed: ${this.resizeWidth}x${this.resizeHeight}\n`);
  }

  queueTerminalText(text) {
    this.ttyInput.push(...this.encoder.encode(text));
    this.updateInputStatus();
    this.requestInputPump?.("tty");
  }

  consoleWrite(ptr, len) {
    const bytes = new Uint8Array(this.memory.buffer, ptr, len);
    const text = this.decoder.decode(bytes.slice()).replace(/\x1b\[[0-9;]*[A-Za-z]/g, "");
    this.log(text);
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
    clearTimeout(this.persistTimer);
    this.persistTimer = setTimeout(() => this.persistRootfs?.(this.rootfs.slice()), 250);
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
    new Uint8Array(this.memory.buffer, ptr, count).set(this.ttyInput.splice(0, count));
    this.updateInputStatus();
    return count;
  }

  framebufferConfigure(width, height) {
    if (width <= 0 || height <= 0) return -1;
    this.framebufferWidth = width;
    this.framebufferHeight = height;
    postMessage({ type: "framebuffer-configure", width, height });
    return 0;
  }

  framebufferFlush(ptr, width, height) {
    if (width !== this.framebufferWidth || height !== this.framebufferHeight ||
        ptr + width * height * 4 > this.memory.buffer.byteLength) return -1;
    /* The guest starts composing the next frame as soon as this Host call
     * returns. Snapshot now so the asynchronous compositor can never sample
     * the shared framebuffer halfway through that next composition pass. */
    const byteLength = width * height * 4;
    let buffer = this.framebufferBuffers.pop();
    if (!(buffer instanceof ArrayBuffer) || buffer.byteLength !== byteLength)
      buffer = new ArrayBuffer(byteLength);
    const pixels = new Uint8Array(buffer);
    pixels.set(new Uint8Array(this.memory.buffer, ptr, byteLength));
    postMessage(
      { type: "framebuffer-flush", width, height, pixels: pixels.buffer },
      [pixels.buffer],
    );
    return 0;
  }

  updateInputStatus() {
    postMessage({
      type: "input-status",
      text: `Keyboard ${this.keyEventCount} · pointer ${this.mouseEventCount} · audio ${this.audioSampleCount} · tty ${this.ttyInput.length} · net ${this.netTxCount}/${this.netRxCount}`,
    });
  }

  netWrite(ptr, len) {
    if (len < 14 || len > 65535) return -1;
    const frame = new Uint8Array(this.memory.buffer, ptr, len).slice();
    if (this.netSocket?.readyState === WebSocket.OPEN) this.netSocket.send(frame);
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
    const samples = new Uint8Array(this.memory.buffer, ptr, size).slice();
    this.audioSampleCount += size / (Math.abs(bitDepth) / 8) / channels;
    this.updateInputStatus();
    postMessage(
      { type: "audio", samples: samples.buffer, rate, channels, bitDepth },
      [samples.buffer],
    );
    return size;
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
        wasm_host_halt: () => { throw new WebAssembly.RuntimeError("EwokOS halted"); },
        wasm_host_block_read: () => -1,
        wasm_host_block_write: (sector, ptr, count) => this.blockWrite(sector, ptr, count),
        wasm_host_block_flush: () => 0,
        wasm_host_ready: () => { this.ready = true; },
      },
    };
  }
}

let host = null;
const pendingHostMessages = [];
self.addEventListener("message", (event) => {
  if (host) host.handleMessage(event.data);
  else pendingHostMessages.push(event.data);
});

(async () => {
  try {
    const memory = new WebAssembly.Memory({
      initial: 14336,
      maximum: 14336,
      shared: true,
    });
    const rootfsState = await loadPersistentRootfs("rootfs.img")
      .catch(() => ({ image: null, persist: null }));
    const netRelay = new URLSearchParams(location.search).get("netRelay");
    host = new WorkerEwokHost(memory, rootfsState.image, rootfsState.persist, netRelay);
    for (const message of pendingHostMessages.splice(0))
      host.handleMessage(message);
    await bootEwokRuntime(host, memory);
  } catch (error) {
    postMessage({ type: "fatal", message: error.stack || error.message });
  }
})();
