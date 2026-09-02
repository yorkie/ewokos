import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";

const bytes = await readFile(new URL("kernel/kernel.wasm", import.meta.url));
const initBytes = await readFile(new URL("user/init.wasm", import.meta.url));
const workerBytes = await readFile(new URL("user/worker.wasm", import.meta.url));
const coreBytes = await readFile(new URL("user/core.wasm", import.meta.url));
const vfsdBytes = await readFile(new URL("user/vfsd.wasm", import.meta.url));
const browserRuntime = await readFile(new URL("runtime.js", import.meta.url), "utf8");
const workerRuntime = await readFile(new URL("runtime-worker.js", import.meta.url), "utf8");
const compositorRuntime = await readFile(
  new URL("compositor-worker.js", import.meta.url),
  "utf8",
);
const processRuntime = await readFile(new URL("process-worker.js", import.meta.url), "utf8");
const browserServer = await readFile(new URL("serve.py", import.meta.url), "utf8");
const xinputSource = await readFile(
  new URL("../system/xwin/drivers/xserverd/xinput.c", import.meta.url),
  "utf8",
);
const xrepaintSource = await readFile(
  new URL("../system/xwin/drivers/xserverd/xrepaint.c", import.meta.url),
  "utf8",
);
const keyboardClientSources = await Promise.all(
  ["wdemo.c", "ximg.c", "xread.c", "xterm.c"].map((name) =>
    readFile(new URL(`user/${name}`, import.meta.url), "utf8")),
);
const manifest = JSON.parse(
  await readFile(new URL("user/modules.json", import.meta.url), "utf8"),
);
assert.deepEqual(
  manifest.modules.map(({ path, url }) => ({ path, url })),
  [
    { path: "/sbin/init", url: "user/init.wasm" },
    { path: "/sbin/worker.wasm", url: "user/worker.wasm" },
    { path: "/sbin/core", url: "user/core.wasm" },
    { path: "/sbin/vfsd", url: "user/vfsd.wasm" },
    { path: "/sbin/sdfsd", url: "user/sdfsd.wasm" },
    { path: "/sbin/sessiond", url: "user/sessiond.wasm" },
    { path: "/drivers/wasm/ttyd", url: "user/ttyd.wasm" },
    { path: "/drivers/logd", url: "user/logd.wasm" },
    { path: "/drivers/wasm/soundd", url: "user/soundd.wasm" },
    { path: "/bin/sound_probe", url: "user/sound_probe.wasm" },
    { path: "/drivers/wasm/ethd", url: "user/ethd.wasm" },
    { path: "/drivers/netd", url: "user/netd.wasm" },
    { path: "/bin/network_probe", url: "user/network_probe.wasm" },
    { path: "/drivers/timed", url: "user/timed.wasm" },
    { path: "/bin/time_probe", url: "user/time_probe.wasm" },
    { path: "/drivers/wasm/powerd", url: "user/powerd.wasm" },
    { path: "/bin/power_probe", url: "user/power_probe.wasm" },
    { path: "/bin/persistence_probe", url: "user/persistence_probe.wasm" },
    { path: "/bin/png_probe", url: "user/png_probe.wasm" },
    { path: "/drivers/nulld", url: "user/nulld.wasm" },
    { path: "/drivers/ramfsd", url: "user/ramfsd.wasm" },
    { path: "/drivers/piped", url: "user/piped.wasm" },
    { path: "/drivers/timerd", url: "user/timerd.wasm" },
    { path: "/drivers/g2dd", url: "user/g2dd.wasm" },
    { path: "/bin/g2d_probe", url: "user/g2d_probe.wasm" },
    { path: "/drivers/displaymand", url: "user/displaymand.wasm" },
    { path: "/drivers/wasm/fbdisplayd", url: "user/fbdisplayd.wasm" },
    { path: "/drivers/wasm/keybd", url: "user/keybd.wasm" },
    { path: "/drivers/wasm/moused", url: "user/moused.wasm" },
    { path: "/bin/input_probe", url: "user/input_probe.wasm" },
    { path: "/drivers/fontd", url: "user/fontd.wasm" },
    { path: "/bin/font_probe", url: "user/font_probe.wasm" },
    { path: "/sbin/splashd", url: "user/splashd.wasm" },
    { path: "/bin/splash", url: "user/splash.wasm" },
    { path: "/drivers/xserverd", url: "user/xserverd.wasm" },
    { path: "/sbin/x/xwm_ewok", url: "user/xwm_ewok.wasm" },
    { path: "/bin/x/xlauncher", url: "user/xdesktop.wasm" },
    { path: "/sbin/x/xmouse", url: "user/xmouse.wasm" },
    { path: "/sbin/x/xim_none", url: "user/xim_none.wasm" },
    { path: "/apps/xDemo/xDemo", url: "user/xdemo.wasm" },
    { path: "/apps/xcores/xcores", url: "user/xcores.wasm" },
    { path: "/apps/xapps/xapps", url: "user/xapps.wasm" },
    { path: "/apps/xfinder/xfinder", url: "user/xfinder.wasm" },
    { path: "/apps/ximg/ximg", url: "user/ximg.wasm" },
    { path: "/apps/xread/xread", url: "user/xread.wasm" },
    { path: "/apps/xg2dtest/xg2dtest", url: "user/xg2dtest.wasm" },
    { path: "/apps/wDemo/wDemo", url: "user/wdemo.wasm" },
    { path: "/apps/xfonts/xfonts", url: "user/xfonts.wasm" },
    { path: "/apps/xtheme/xtheme", url: "user/xtheme.wasm" },
    { path: "/apps/xwifi/xwifi", url: "user/xwifi.wasm" },
    { path: "/apps/SndPlayer/SndPlayer", url: "user/sndplayer.wasm" },
    { path: "/apps/clock/clock", url: "user/clock.wasm" },
    { path: "/apps/xlog/xlog", url: "user/xlog.wasm" },
    { path: "/apps/xprocs/xprocs", url: "user/xprocs.wasm" },
    { path: "/apps/xwm_theme/xwm_theme", url: "user/xwm_theme.wasm" },
    { path: "/apps/xterm/xterm", url: "user/xterm.wasm" },
    { path: "/bin/display_probe", url: "user/display_probe.wasm" },
    { path: "/bin/rootfs_probe", url: "user/rootfs_probe.wasm" },
    { path: "/bin/cat", url: "user/cat.wasm" },
    { path: "/bin/uname", url: "user/uname.wasm" },
    { path: "/bin/shell", url: "user/shell.wasm" },
  ],
);
const allowedUserImports = new Set([
  "memory",
  "ewok_syscall",
  "wasm_host_block_read",
  "wasm_host_block_write",
  "wasm_host_block_flush",
  "wasm_host_tty_read",
  "wasm_host_tty_write",
  "wasm_host_tty_available",
  "wasm_host_tty_input",
  "wasm_host_spawn_command",
  "wasm_host_launch_argument",
  "wasm_host_launch_generation",
  "wasm_host_framebuffer_configure",
  "wasm_host_framebuffer_flush",
  "wasm_host_key_read",
  "wasm_host_key_available",
  "wasm_host_mouse_read",
  "wasm_host_mouse_available",
  "wasm_host_audio_write",
  "wasm_host_unix_time_sec",
  "wasm_host_net_read",
  "wasm_host_net_write",
  "wasm_host_net_available",
  // C library math primitives used by the native libvorbis decoder.
  "cos",
  "sin",
  "log",
  "rint",
  "exp",
  "atan",
  "floor",
  "ldexp",
  "pow",
  "ceil",
]);
for (const descriptor of manifest.modules) {
  const userBytes = await readFile(new URL(descriptor.url, import.meta.url));
  const userModule = await WebAssembly.compile(userBytes);
  for (const imported of WebAssembly.Module.imports(userModule)) {
    assert.equal(imported.module, "env", `${descriptor.path} imported WASI`);
    assert.ok(
      allowedUserImports.has(imported.name),
      `${descriptor.path} has undocumented Host import ${imported.name}`,
    );
  }
  if (descriptor.path === "/drivers/wasm/fbdisplayd") {
    assert.ok(
      WebAssembly.Module.exports(userModule).some(
        ({ name, kind }) => name === "ewok_display_resize" && kind === "function",
      ),
      "browser display driver must export its resize entry point",
    );
  }
  if (descriptor.path === "/apps/xterm/xterm") {
    assert.ok(
      WebAssembly.Module.exports(userModule).some(
        ({ name, kind }) => name === "ewok_xterm_write" && kind === "function",
      ),
      "xterm must export its tty output bridge",
    );
    assert.equal(descriptor.hostPoll, true, "xterm must poll X and tty events");
  }
}
assert.equal(
  manifest.modules.find(({ path }) => path === "/bin/shell")?.hostPoll,
  true,
  "interactive shell must keep polling browser terminal input",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/drivers/xserverd")?.pollIntervalMs,
  16,
  "xserver must retain frame-rate polling",
);
assert.ok(
  manifest.modules.find(({ path }) => path === "/bin/x/xlauncher")?.pollIntervalMs >= 250,
  "desktop background polling must not repeat expensive display queries each frame",
);
assert.match(browserRuntime, /new Worker\(workerUrl/);
assert.match(browserRuntime, /executionMode: "worker-pipeline"/);
assert.match(browserRuntime, /const queueBitmap = \(bitmap\) =>/);
assert.match(browserRuntime, /if \(pendingBitmap\) pendingBitmap\.close\(\)/);
assert.match(browserRuntime, /requestAnimationFrame\(\(\) =>/);
assert.match(browserRuntime, /event\.data\.type === "framebuffer"\) queueBitmap/);
assert.match(
  xinputSource,
  /if\(win != x->win_tail\)\s*xwin_top\(x, win\);[\s\S]*?try_focus\(x, win\);/,
  "a pointer press must raise and focus an overlapped window atomically",
);
assert.match(
  xrepaintSource,
  /grect_t overlap = win->xinfo->winr;[\s\S]*?grect_insect\(&dirty_rects\[i\], &overlap\)[\s\S]*?win->dirty = true;/,
  "damage from a lower window must force overlapping upper windows into the same composite pass",
);
assert.ok(
  (browserRuntime.match(/addEventListener\("wheel"/g) || []).length >= 2,
  "both browser execution paths must forward wheel input",
);
assert.match(browserRuntime, /passive: false/);
assert.match(workerRuntime, /shared: true/);
assert.match(workerRuntime, /bootEwokRuntime\(host, memory\)/);
assert.match(workerRuntime, /pendingHostMessages\.splice\(0\)/);
assert.match(
  workerRuntime,
  /pixels\.set\(new Uint8Array\(this\.memory\.buffer, ptr, byteLength\)\)/,
  "framebuffer flush must snapshot shared memory before returning to the guest",
);
assert.match(workerRuntime, /this\.framebufferBuffers\.pop\(\)/);
assert.match(compositorRuntime, /new Uint8Array\(memory\.buffer/);
assert.match(compositorRuntime, /frame\.pixels instanceof ArrayBuffer/);
assert.match(compositorRuntime, /type: "framebuffer-buffer"/);
assert.match(compositorRuntime, /getContext\("webgl2"/);
assert.match(compositorRuntime, /\.bgra/);
assert.match(compositorRuntime, /mode: "canvas2d"/);
assert.match(processRuntime, /Atomics\.wait/);
assert.match(processRuntime, /ewok_syscall: syscall/);
assert.match(processRuntime, /syscall\(WASM_HOSTCALL_SPAWN_COMMAND, ptr, len, 0\)/);
assert.match(browserRuntime, /code === WASM_HOSTCALL_SPAWN_COMMAND/);
assert.match(browserRuntime, /const kernelTraceCallers = new Int32Array\(64\)/);
assert.match(browserRuntime, /recentKernelTrace\(16\)/);
assert.doesNotMatch(browserRuntime, /kernelTrace\.push/);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xapps/xapps")?.worker,
  true,
  "xapps must not block the kernel worker while painting or handling X events",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xprocs/xprocs")?.worker,
  true,
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xcores/xcores")?.worker,
  true,
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xDemo/xDemo")?.worker,
  true,
  "animated demo should not consume the kernel worker frame budget",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xwm_theme/xwm_theme")?.worker,
  true,
  "theme editor must not block the kernel worker while repainting",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xfinder/xfinder")?.worker,
  true,
  "file manager must not block the kernel worker while reading directories",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/ximg/ximg")?.worker,
  true,
  "image viewer must not block the kernel worker while decoding images",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xread/xread")?.worker,
  true,
  "text reader must not block the kernel worker while rendering text",
);
assert.equal(
  manifest.modules.find(({ path }) => path === "/apps/xg2dtest/xg2dtest")?.worker,
  true,
  "g2d validation must not block the kernel worker while benchmarking",
);
for (const path of [
  "/apps/wDemo/wDemo",
  "/apps/xfonts/xfonts",
  "/apps/xtheme/xtheme",
  "/apps/xwifi/xwifi",
  "/apps/SndPlayer/SndPlayer",
]) {
  assert.equal(
    manifest.modules.find((module) => module.path === path)?.worker,
    true,
    `${path} must run outside the kernel worker`,
  );
}
assert.match(browserRuntime, /splitLaunchCommand\(commandLine\)/);
assert.match(processRuntime, /wasm_host_launch_argument: copyLaunchArgument/);
assert.match(browserServer, /Cross-Origin-Opener-Policy/);
assert.match(browserServer, /Cross-Origin-Embedder-Policy/);
for (const source of keyboardClientSources) {
  assert.doesNotMatch(
    source,
    /XWIN_STYLE_XIM/,
    "regular keyboard clients must not register themselves as the input-method window",
  );
  assert.match(source, /xwin_call_xim\(/);
}
const module = await WebAssembly.compile(bytes);
const memory = new WebAssembly.Memory({
  initial: 14336,
  maximum: 14336,
  shared: true,
});
assert.ok(memory.buffer instanceof SharedArrayBuffer);
const decoder = new TextDecoder();
let output = "";
let ready = false;

assert.deepEqual(
  WebAssembly.Module.imports(module).map(({ module, name, kind }) => ({
    module,
    name,
    kind,
  })),
  [
    { module: "env", name: "memory", kind: "memory" },
    { module: "env", name: "wasm_host_now_usec", kind: "function" },
    { module: "env", name: "wasm_host_halt", kind: "function" },
    { module: "env", name: "wasm_host_ready", kind: "function" },
    { module: "env", name: "console_write", kind: "function" },
    { module: "env", name: "wasm_host_block_read", kind: "function" },
  ],
  "kernel.wasm must only depend on the documented browser Host ABI",
);

const instance = await WebAssembly.instantiate(module, {
  env: {
    memory,
    wasm_host_now_usec: () => BigInt(Math.floor(performance.now() * 1000)),
    wasm_host_halt: () => {
      throw new Error("EwokOS halted during boot");
    },
    wasm_host_ready: () => {
      ready = true;
    },
    console_write: (ptr, length) => {
      output += decoder.decode(new Uint8Array(memory.buffer, ptr, length));
    },
    wasm_host_block_read: () => -1,
  },
});

instance.exports._start();
assert.equal(ready, true, "kernel did not reach the wasm-ready boundary");
assert.match(output, /machine\s+wasm/);
assert.match(output, /kernel: wasm core ready/);
assert.equal(instance.exports.wasm_mmu_self_test(), 0, "software MMU failed");
assert.equal(instance.exports.wasm_kernel_physical_limit(), 0x08000000);
assert.equal(instance.exports.wasm_kernel_module_limit(), 0x22000000 - 0x1000);
assert.equal(instance.exports.wasm_kernel_shm_base(), 0x24000000 - 0x1000);
assert.equal(instance.exports.wasm_kernel_stack_bottom(), 0x2c000000 - 0x1000);
assert.equal(instance.exports.wasm_kernel_base(), 0x30000000);

const initModule = await WebAssembly.compile(initBytes);
let initPid = -1;
const init = await WebAssembly.instantiate(initModule, {
  env: {
    memory,
    ewok_syscall: (code, arg0, arg1, arg2) => {
      if (instance.exports.wasm_kernel_activate(initPid) !== 0) return -1;
      return instance.exports.wasm_kernel_syscall(code, arg0, arg1, arg2);
    },
    wasm_host_spawn_command: () => -1,
  },
});
const command = init.exports.ewok_init_command();
const commandSize = init.exports.ewok_init_command_size();
const moduleBase = init.exports.ewok_module_base();
const moduleSize = init.exports.ewok_module_size();
initPid = instance.exports.wasm_kernel_spawn(
  command,
  commandSize,
  moduleBase,
  moduleSize,
  init.exports.ewok_heap_base(),
);
assert.ok(initPid >= 0, "kernel could not create init.wasm process metadata");
assert.equal(instance.exports.wasm_kernel_set_system_process(initPid), 0);
assert.equal(init.exports._start(), initPid, "SYS_GET_PID did not use init context");
assert.match(output, /\[init process started\]/);
const initProcInfo = moduleBase + moduleSize - 512;
assert.equal(instance.exports.wasm_kernel_activate(initPid), 0);
assert.equal(instance.exports.wasm_kernel_syscall(32, initPid, initProcInfo, 0), 0);
const reportedInitHeap = new DataView(memory.buffer).getUint32(
  initProcInfo + 44,
  true,
);
assert.ok(
  reportedInitHeap < moduleSize,
  `procinfo heap must be an arena-relative size, got 0x${reportedInitHeap.toString(16)}`,
);

const workerModule = await WebAssembly.compile(workerBytes);
let workerPid = -1;
const worker = await WebAssembly.instantiate(workerModule, {
  env: {
    memory,
    ewok_syscall: (code, arg0, arg1, arg2) => {
      if (instance.exports.wasm_kernel_activate(workerPid) !== 0) return -1;
      return instance.exports.wasm_kernel_syscall(code, arg0, arg1, arg2);
    },
  },
});
workerPid = instance.exports.wasm_kernel_spawn(
  worker.exports.ewok_init_command(),
  worker.exports.ewok_init_command_size(),
  worker.exports.ewok_module_base(),
  worker.exports.ewok_module_size(),
  worker.exports.ewok_heap_base(),
);
assert.ok(workerPid > initPid, "kernel did not allocate a second process");
assert.equal(worker.exports._start(), workerPid);
assert.match(output, /worker\.wasm: entered isolated EwokOS process/);

assert.equal(instance.exports.wasm_kernel_activate(initPid), 0);
assert.equal(
  instance.exports.wasm_kernel_syscall(
    1,
    worker.exports.ewok_module_base(),
    16,
    0,
  ),
  -1,
  "software MMU allowed init to read the worker module",
);
init.exports.ewok_step();
worker.exports.ewok_step();
assert.match(output, /worker\.wasm: cooperative step/);
assert.match(output, /worker\.wasm: read sysinfo through software MMU/);
assert.equal(worker.exports.ewok_memory_mb(), 128);
assert.equal(instance.exports.wasm_kernel_activate(workerPid), 0);
const workerHeap = instance.exports.wasm_kernel_syscall(2, 4096, 0, 0);
assert.ok(
  workerHeap >= worker.exports.ewok_heap_base() &&
    workerHeap + 4096 <=
      worker.exports.ewok_module_base() + worker.exports.ewok_module_size(),
  "worker heap escaped its reserved wasm arena",
);
const remaining =
  worker.exports.ewok_module_base() +
  worker.exports.ewok_module_size() -
  workerHeap -
  4096;
assert.ok(remaining > 0);
assert.equal(instance.exports.wasm_kernel_syscall(2, remaining, 0, 0), workerHeap);
assert.equal(
  instance.exports.wasm_kernel_syscall(2, 4096, 0, 0),
  0,
  "kernel allowed the worker heap to cross its wasm arena",
);
const uptimeBefore = instance.exports.wasm_kernel_uptime_usec();
instance.exports.wasm_kernel_tick(2500);
assert.equal(instance.exports.wasm_kernel_uptime_usec(), uptimeBefore + 2500n);
assert.equal(instance.exports.wasm_kernel_runnable(initPid), 1);
assert.equal(instance.exports.wasm_kernel_runnable(workerPid), 1);

const coreModule = await WebAssembly.compile(coreBytes);
assert.deepEqual(
  WebAssembly.Module.imports(coreModule).map(({ module, name, kind }) => ({
    module,
    name,
    kind,
  })),
  [
    { module: "env", name: "memory", kind: "memory" },
    { module: "env", name: "ewok_syscall", kind: "function" },
  ],
  "core.wasm must use EwokOS syscalls rather than WASI or host libc",
);
let corePid = -1;
const core = await WebAssembly.instantiate(coreModule, {
  env: {
    memory,
    ewok_syscall: (code, arg0, arg1, arg2) => {
      if (instance.exports.wasm_kernel_activate(corePid) !== 0) return -1;
      return instance.exports.wasm_kernel_syscall(code, arg0, arg1, arg2);
    },
  },
});
corePid = instance.exports.wasm_kernel_spawn(
  core.exports.ewok_init_command(),
  core.exports.ewok_init_command_size(),
  core.exports.ewok_module_base(),
  core.exports.ewok_module_size(),
  core.exports.ewok_heap_base(),
);
assert.ok(corePid > workerPid, "kernel did not allocate the real core service");
assert.equal(
  core.exports._start(),
  corePid,
  `core service failed at entry stage ${core.exports.ewok_start_stage()}, core stage ${core.exports.ewok_core_stage()}`,
);
core.exports.ewok_step();
assert.equal(instance.exports.wasm_kernel_runnable(corePid), 1);

const vfsdModule = await WebAssembly.compile(vfsdBytes);
assert.deepEqual(
  WebAssembly.Module.imports(vfsdModule).map(({ module, name, kind }) => ({
    module,
    name,
    kind,
  })),
  [
    { module: "env", name: "memory", kind: "memory" },
    { module: "env", name: "ewok_syscall", kind: "function" },
  ],
  "vfsd.wasm must use the native EwokOS syscall ABI only",
);

console.log(
  `PASS kernel booted, software MMU isolated pid=${initPid}/${workerPid}, core=${corePid}`,
);
