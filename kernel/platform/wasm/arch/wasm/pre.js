// Pre-JS code for ewokos WASM integration
var ewokos = {
    console_output: '',
    timer_interval: null,
    
    // Initialize ewokos environment
    init: function() {
        console.log('Initializing ewokos WASM environment...');
        
        // Set up timer for scheduling
        this.timer_interval = setInterval(function() {
            if (Module._wasm_timer_interrupt) {
                Module._wasm_timer_interrupt();
            }
        }, 10); // 100Hz timer
        
        // Redirect stdout to our console
        Module.print = function(text) {
            ewokos.console_output += text + '\n';
            ewokos.updateConsole();
        };
        
        Module.printErr = function(text) {
            ewokos.console_output += 'ERROR: ' + text + '\n';
            ewokos.updateConsole();
        };
    },
    
    // Update the HTML console display
    updateConsole: function() {
        var consoleElement = document.getElementById('ewokos-console');
        if (consoleElement) {
            consoleElement.textContent = this.console_output;
            consoleElement.scrollTop = consoleElement.scrollHeight;
        }
    },
    
    // Send keyboard input to ewokos
    sendKey: function(keyCode) {
        if (Module._wasm_keyboard_input) {
            Module._wasm_keyboard_input(keyCode);
        }
    },
    
    // Clear console
    clearConsole: function() {
        this.console_output = '';
        this.updateConsole();
    }
};

// Module configuration
Module = {
    onRuntimeInitialized: function() {
        console.log('ewokos WASM runtime initialized');
        ewokos.init();
        
        // Start the kernel
        if (Module._emscripten_main) {
            Module._emscripten_main();
        }
    },
    
    print: function(text) {
        console.log('ewokos: ' + text);
    },
    
    printErr: function(text) {
        console.error('ewokos error: ' + text);
    }
};