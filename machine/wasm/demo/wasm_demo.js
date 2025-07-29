// Pre-JS code for EwokOS WASM Demo
var ewokos_demo = {
  console_element: null,
  
  init: function() {
    console.log('Initializing EwokOS WASM Demo...');
    this.console_element = document.getElementById('ewokos-console');
    this.updateConsole();
  },
  
  updateConsole: function() {
    if (Module._get_console_output && this.console_element) {
      var outputPtr = Module._get_console_output();
      var output = UTF8ToString(outputPtr);
      this.console_element.textContent = output;
      this.console_element.scrollTop = this.console_element.scrollHeight;
    }
  },
  
  sendCommand: function(cmd) {
    if (Module._demo_command) {
      var len = lengthBytesUTF8(cmd) + 1;
      var cmdPtr = _malloc(len);
      stringToUTF8(cmd, cmdPtr, len);
      Module._demo_command(cmdPtr);
      _free(cmdPtr);
      this.updateConsole();
    }
  },
  
  clearConsole: function() {
    if (Module._clear_console) {
      Module._clear_console();
      this.updateConsole();
    }
  }
};

// Module configuration
Module = {
  onRuntimeInitialized: function() {
    console.log('EwokOS WASM Demo runtime initialized');
    ewokos_demo.init();
    
    // Run the main function
    if (Module._main) {
      Module._main();
      ewokos_demo.updateConsole();
    }
  },
  
  print: function(text) {
    console.log('ewokos: ' + text);
  },
  
  printErr: function(text) {
    console.error('ewokos error: ' + text);
  }
};