const fs = require('fs');
const path = require('path');

class EwokOSWASMRuntime {
    constructor() {
        this.startTime = Date.now();
        this.uartBuffer = '';
        this.sdImage = null;
        this.initProcess = null;
        this.blockSize = 512; // SD block size
    }

    // WASM import functions that the kernel can call
    getImports() {
        return {
            env: {
                // Debug/console output
                wasm_debug_print: (strPtr, len) => {
                    const memory = this.instance.exports.memory;
                    const str = this.readString(memory, strPtr, len);
                    process.stdout.write(str);
                },

                // Time functions
                wasm_get_time_ms: () => {
                    return Date.now() - this.startTime;
                },

                // Exit function
                wasm_exit: (code) => {
                    console.log(`\nEwokOS exited with code: ${code}`);
                    process.exit(code);
                },

                // UART functions
                wasm_uart_has_data: () => {
                    return this.uartBuffer.length > 0 ? 1 : 0;
                },

                wasm_uart_read_char: () => {
                    if (this.uartBuffer.length > 0) {
                        const char = this.uartBuffer.charCodeAt(0);
                        this.uartBuffer = this.uartBuffer.substring(1);
                        return char;
                    }
                    return 0;
                },

                // SD card functions
                wasm_sd_read_block: (block, bufferPtr, size) => {
                    if (!this.sdImage || size !== this.blockSize) {
                        return -1;
                    }
                    
                    const offset = block * this.blockSize;
                    if (offset >= this.sdImage.length) {
                        return -1;
                    }
                    
                    const memory = this.instance.exports.memory;
                    const buffer = new Uint8Array(memory.buffer, bufferPtr, size);
                    const dataLength = Math.min(size, this.sdImage.length - offset);
                    
                    for (let i = 0; i < dataLength; i++) {
                        buffer[i] = this.sdImage[offset + i];
                    }
                    
                    return 0;
                },

                wasm_sd_write_block: (block, bufferPtr, size) => {
                    if (!this.sdImage || size !== this.blockSize) {
                        return -1;
                    }
                    
                    const offset = block * this.blockSize;
                    if (offset >= this.sdImage.length) {
                        return -1;
                    }
                    
                    const memory = this.instance.exports.memory;
                    const buffer = new Uint8Array(memory.buffer, bufferPtr, size);
                    
                    for (let i = 0; i < size && offset + i < this.sdImage.length; i++) {
                        this.sdImage[offset + i] = buffer[i];
                    }
                    
                    return 0;
                },

                wasm_sd_get_size: () => {
                    return this.sdImage ? Math.floor(this.sdImage.length / this.blockSize) : 0;
                },

                // Init process loading
                wasm_load_init_process: (pathPtr, elfDataPtr, sizePtr) => {
                    const memory = this.instance.exports.memory;
                    const pathStr = this.readCString(memory, pathPtr);
                    
                    console.log(`Loading init process: ${pathStr}`);
                    
                    if (this.initProcess) {
                        // Write ELF data pointer and size
                        const view = new DataView(memory.buffer);
                        view.setUint32(elfDataPtr, this.initProcess.ptr, true);
                        view.setUint32(sizePtr, this.initProcess.size, true);
                        return 0;
                    }
                    
                    return -1;
                }
            }
        };
    }

    // Helper function to read string from WASM memory
    readString(memory, ptr, len) {
        const bytes = new Uint8Array(memory.buffer, ptr, len);
        return new TextDecoder().decode(bytes);
    }

    // Helper function to read C string from WASM memory
    readCString(memory, ptr) {
        const bytes = new Uint8Array(memory.buffer);
        let len = 0;
        while (bytes[ptr + len] !== 0) {
            len++;
        }
        return new TextDecoder().decode(bytes.slice(ptr, ptr + len));
    }

    // Load SD image file
    loadSDImage(imagePath) {
        try {
            this.sdImage = fs.readFileSync(imagePath);
            console.log(`Loaded SD image: ${imagePath} (${this.sdImage.length} bytes)`);
        } catch (error) {
            console.log(`Warning: Could not load SD image ${imagePath}: ${error.message}`);
            // Create a minimal empty SD image
            this.sdImage = Buffer.alloc(16 * 1024 * 1024); // 16MB empty image
        }
    }

    // Load init process
    loadInitProcess(initPath) {
        try {
            const data = fs.readFileSync(initPath);
            // Allocate memory in WASM for the init process
            // This is simplified - in reality we'd need proper memory management
            this.initProcess = {
                data: data,
                ptr: 0x1000000, // Fake pointer
                size: data.length
            };
            console.log(`Loaded init process: ${initPath} (${data.length} bytes)`);
        } catch (error) {
            console.log(`Warning: Could not load init process ${initPath}: ${error.message}`);
        }
    }

    // Start UART input handling
    startUARTInput() {
        process.stdin.setRawMode(true);
        process.stdin.setEncoding('utf8');
        process.stdin.on('data', (key) => {
            if (key === '\u0003') { // Ctrl+C
                process.exit();
            }
            this.uartBuffer += key;
            
            // Trigger UART interrupt if available
            if (this.instance && this.instance.exports.wasm_uart_interrupt) {
                this.instance.exports.wasm_uart_interrupt();
            }
        });
    }

    // Start timer interrupts
    startTimer() {
        setInterval(() => {
            if (this.instance && this.instance.exports.wasm_timer_tick) {
                this.instance.exports.wasm_timer_tick();
            }
        }, 10); // 10ms timer
    }

    // Load and run WASM kernel
    async run(wasmPath) {
        try {
            const wasmBytes = fs.readFileSync(wasmPath);
            const wasmModule = await WebAssembly.compile(wasmBytes);
            this.instance = await WebAssembly.instantiate(wasmModule, this.getImports());

            console.log('EwokOS WASM Runtime starting...');
            
            // Start services
            this.startUARTInput();
            this.startTimer();
            
            // Call the WASM entry point
            if (this.instance.exports._start) {
                this.instance.exports._start();
            } else {
                console.error('No _start export found in WASM module');
            }
            
        } catch (error) {
            console.error('Error running WASM kernel:', error);
            process.exit(1);
        }
    }
}

// Main function
async function main() {
    const args = process.argv.slice(2);
    if (args.length < 1) {
        console.log('Usage: node wasm_runtime.js <kernel.wasm> [sd_image] [init_process]');
        process.exit(1);
    }

    const runtime = new EwokOSWASMRuntime();
    
    // Load optional SD image
    if (args[1]) {
        runtime.loadSDImage(args[1]);
    }
    
    // Load optional init process
    if (args[2]) {
        runtime.loadInitProcess(args[2]);
    }
    
    // Run the kernel
    await runtime.run(args[0]);
}

if (require.main === module) {
    main().catch(console.error);
}

module.exports = EwokOSWASMRuntime;