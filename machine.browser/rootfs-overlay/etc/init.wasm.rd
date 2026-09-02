# Browser-native EwokOS smoke boot. Commands are resolved through the wasm
# module registry and still execute as isolated EwokOS kernel processes.
@export TZ=CST-8
@/bin/font_probe
@/bin/display_probe
@/bin/uname -a
@/bin/cat /etc/init.rd
