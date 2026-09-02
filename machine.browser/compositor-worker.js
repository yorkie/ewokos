let memory = null;
let width = 0;
let height = 0;
let canvas = null;
let context = null;
let gl = null;
let texture = null;
let rgba = null;
let copyTextureUpload = false;
let pending = null;
let queued = false;

function compileShader(type, source) {
  const shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS))
    throw new Error(gl.getShaderInfoLog(shader));
  return shader;
}

function configureWebGL() {
  gl = canvas.getContext("webgl2", {
    alpha: false,
    antialias: false,
    depth: false,
    stencil: false,
  });
  if (!gl) return false;

  const program = gl.createProgram();
  gl.attachShader(program, compileShader(gl.VERTEX_SHADER, `#version 300 es
    in vec2 position;
    in vec2 texcoord;
    out vec2 uv;
    void main() {
      gl_Position = vec4(position, 0.0, 1.0);
      uv = texcoord;
    }
  `));
  gl.attachShader(program, compileShader(gl.FRAGMENT_SHADER, `#version 300 es
    precision mediump float;
    uniform sampler2D framebuffer;
    in vec2 uv;
    out vec4 color;
    void main() {
      color = texture(framebuffer, uv).bgra;
    }
  `));
  gl.linkProgram(program);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS))
    throw new Error(gl.getProgramInfoLog(program));
  gl.useProgram(program);

  const vertices = new Float32Array([
    -1, -1, 0, 1,
     1, -1, 1, 1,
    -1,  1, 0, 0,
     1,  1, 1, 0,
  ]);
  const buffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
  gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
  const position = gl.getAttribLocation(program, "position");
  const texcoord = gl.getAttribLocation(program, "texcoord");
  gl.enableVertexAttribArray(position);
  gl.vertexAttribPointer(position, 2, gl.FLOAT, false, 16, 0);
  gl.enableVertexAttribArray(texcoord);
  gl.vertexAttribPointer(texcoord, 2, gl.FLOAT, false, 16, 8);

  texture = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texImage2D(
    gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0,
    gl.RGBA, gl.UNSIGNED_BYTE, null,
  );
  gl.viewport(0, 0, width, height);
  return true;
}

function configure(nextWidth, nextHeight) {
  width = nextWidth;
  height = nextHeight;
  canvas = new OffscreenCanvas(width, height);
  context = null;
  gl = null;
  texture = null;
  rgba = null;
  copyTextureUpload = false;
  try {
    if (configureWebGL()) {
      postMessage({ type: "compositor-mode", mode: "webgl2" });
      return;
    }
  } catch {
    canvas = new OffscreenCanvas(width, height);
    gl = null;
    texture = null;
  }
  context = canvas.getContext("2d");
  rgba = new Uint8ClampedArray(width * height * 4);
  postMessage({ type: "compositor-mode", mode: "canvas2d" });
}

function queueFlush(message) {
  if (pending?.pixels instanceof ArrayBuffer)
    postMessage(
      { type: "framebuffer-buffer", buffer: pending.pixels },
      [pending.pixels],
    );
  pending = message;
  if (queued) return;
  queued = true;
  setTimeout(flush, 0);
}

function flush() {
  queued = false;
  const frame = pending;
  pending = null;
  if (!frame || !memory || (!gl && !context) ||
      frame.width !== width || frame.height !== height)
    return;
  /* New runtimes transfer an immutable snapshot. The shared-memory fallback
   * keeps old cached runtimes bootable, but is intentionally not used for
   * current frames because the guest may already be drawing the next one. */
  const source = frame.pixels instanceof ArrayBuffer ?
    new Uint8Array(frame.pixels) :
    new Uint8Array(memory.buffer, frame.ptr, width * height * 4);
  if (gl) {
    gl.bindTexture(gl.TEXTURE_2D, texture);
    try {
      gl.texSubImage2D(
        gl.TEXTURE_2D, 0, 0, 0, width, height,
        gl.RGBA, gl.UNSIGNED_BYTE, copyTextureUpload ? source.slice() : source,
      );
    } catch {
      copyTextureUpload = true;
      gl.texSubImage2D(
        gl.TEXTURE_2D, 0, 0, 0, width, height,
        gl.RGBA, gl.UNSIGNED_BYTE, source.slice(),
      );
    }
    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  } else {
    for (let offset = 0; offset < source.length; offset += 4) {
      rgba[offset] = source[offset + 2];
      rgba[offset + 1] = source[offset + 1];
      rgba[offset + 2] = source[offset];
      rgba[offset + 3] = source[offset + 3];
    }
    context.putImageData(new ImageData(rgba, width, height), 0, 0);
  }
  const bitmap = canvas.transferToImageBitmap();
  postMessage({ type: "framebuffer", bitmap }, [bitmap]);
  if (frame.pixels instanceof ArrayBuffer)
    postMessage(
      { type: "framebuffer-buffer", buffer: frame.pixels },
      [frame.pixels],
    );
  if (pending) queueFlush(pending);
}

self.addEventListener("message", (event) => {
  const message = event.data;
  if (message.type === "init") {
    memory = message.memory;
    if (pending) queueFlush(pending);
  } else if (message.type === "configure") {
    configure(message.width, message.height);
    if (pending) queueFlush(pending);
  } else if (message.type === "flush") {
    queueFlush(message);
  }
});
