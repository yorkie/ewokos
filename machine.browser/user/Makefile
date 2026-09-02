LLVM_PREFIX ?= $(if $(wildcard /opt/homebrew/opt/llvm/bin/clang),/opt/homebrew/opt/llvm/bin/,)
CC = $(LLVM_PREFIX)clang --target=wasm32
MODULE_BASE = 0x16f00000
MODULE_SIZE = 0x00200000
GLOBAL_BASE = 385875968

CFLAGS = -nostdlib -ffreestanding -fno-stack-protector -Wall -Wextra \
	-matomics -mbulk-memory \
	-I../../kernel/kernel/include \
	-I../../kernel/platform/wasm/arch/wasm \
	-I../../system/basic/libc/libewoksys/ewoksys/include \
	-DEWOK_WASM_MODULE_BASE=$(MODULE_BASE) \
	-DEWOK_WASM_MODULE_SIZE=$(MODULE_SIZE)
LDFLAGS = -nostdlib -matomics -mbulk-memory \
	-Wl,--no-entry \
	-Wl,--export=_start \
	-Wl,--export=ewok_init_command \
	-Wl,--export=ewok_init_command_size \
	-Wl,--export=ewok_module_base \
	-Wl,--export=ewok_module_size \
	-Wl,--export=ewok_heap_base \
	-Wl,--export=ewok_step \
	-Wl,--export-if-defined=ewok_memory_mb \
	-Wl,--export-if-defined=ewok_launch_argument_changed \
	-Wl,--import-memory \
	-Wl,--shared-memory \
	-Wl,--import-undefined \
	-Wl,--global-base=$(GLOBAL_BASE) \
	-Wl,--no-stack-first \
	-Wl,-z,stack-size=65536 \
	-Wl,--initial-memory=939524096 \
	-Wl,--max-memory=939524096

.PHONY: all clean

all: init.wasm worker.wasm core.wasm vfsd.wasm sdfsd.wasm sessiond.wasm rootfs_probe.wasm \
	ttyd.wasm logd.wasm soundd.wasm sound_probe.wasm ethd.wasm netd.wasm network_probe.wasm \
	timed.wasm time_probe.wasm powerd.wasm power_probe.wasm \
	persistence_probe.wasm png_probe.wasm \
	nulld.wasm ramfsd.wasm piped.wasm timerd.wasm g2dd.wasm g2d_probe.wasm cat.wasm \
	displaymand.wasm fbdisplayd.wasm keybd.wasm moused.wasm display_probe.wasm \
	input_probe.wasm fontd.wasm font_probe.wasm xserverd.wasm xwm_ewok.wasm xmouse.wasm \
	xim_none.wasm splashd.wasm splash.wasm xdesktop.wasm xdemo.wasm xcores.wasm xapps.wasm \
	xfinder.wasm ximg.wasm xread.wasm xg2dtest.wasm wdemo.wasm xfonts.wasm xtheme.wasm xwifi.wasm sndplayer.wasm \
	clock.wasm xlog.wasm xprocs.wasm xwm_theme.wasm \
	xterm.wasm uname.wasm shell.wasm



SYSCALL_SRC = ../../system/basic/libc/libewoksys/ewoksys/src/syscall_wasm.c
EWOKSYS_SOURCES = $(filter-out %/cmain.c %/syscall_wasm.c, \
	$(shell find ../../system/basic/libc/libewoksys/ewoksys/src \
		../../system/basic/libc/libewoksys/src -name '*.c' | sort))

GRAPH_SOURCES = $(filter-out %/arc.c %/graph_png.c %/graph_jpeg.c %/graph_gif.c \
	%/graph_tga.c %/graph_svg.c %/graph_image.c, \
	$(shell find ../../system/gui/libs/graph/src -name '*.c' | sort))
PNG_SOURCES = $(filter-out %/example.c %/pngtest.c, \
	$(shell find ../../system/gui/libs/libpng -maxdepth 1 -name '*.c' | sort))
ZLIB_READ_SOURCES = ../../system/basic/libs/zlib/adler32.c \
	../../system/basic/libs/zlib/crc32.c ../../system/basic/libs/zlib/inffast.c \
	../../system/basic/libs/zlib/inflate.c ../../system/basic/libs/zlib/inftrees.c \
	../../system/basic/libs/zlib/zutil.c
PNG_GRAPH_SOURCES = $(GRAPH_SOURCES) \
	../../system/gui/libs/graph/src/graph_png.c \
	../../system/gui/libs/graph/src/graph_image.c
PNG_CFLAGS = -I../../system/gui/libs/libpng -I../../system/basic/libs/zlib

init.wasm: service_entry.c ../../system/basic/sys/init/init.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/init"' \
		-DEWOK_WASM_MODULE_BASE=0x17000000 \
		-DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=386924544,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

worker.wasm: worker.c $(SYSCALL_SRC)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=%,$(CFLAGS)) \
		-DEWOK_WASM_MODULE_BASE=0x16d00000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=383778816,$(LDFLAGS)) \
		-o $@

SERVICE_CFLAGS = $(CFLAGS) -fno-builtin -std=c99 -O2 \
	-Wno-unused-parameter -Wno-implicit-function-declaration \
	-I../../system/basic/libc/libewoksys/include \
	-Iinclude \
	-I../../system/build/include
SERVICE_LDFLAGS = $(LDFLAGS) \
	-Wl,--export=ewok_start_stage \
	-Wl,--export-if-defined=ewok_core_stage \
	-Wl,--export-if-defined=ewok_sdfsd_stage \
	-Wl,--export-if-defined=ewok_rootfs_probe_result \
	-Wl,--export-if-defined=ewok_program_result \
	-Wl,--export-if-defined=ewok_display_probe_stage \
	-Wl,--export-if-defined=ewok_input_probe_keys \
	-Wl,--export-if-defined=ewok_input_probe_mouse \
	-Wl,--export=ewok_dispatch \
	-Wl,--export=ewok_init_command \
	-Wl,--export=ewok_init_command_size \
	-Wl,--export=ewok_module_base \
	-Wl,--export=ewok_module_size \
	-Wl,--export=ewok_heap_base

core.wasm: service_entry.c ../../system/basic/sys/core/core.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/core"' \
		-DEWOK_WASM_MODULE_BASE=0x14000000 \
		-DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=336592896,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

VFSD_SOURCES = $(shell find ../../system/basic/sys/vfsd -maxdepth 1 -name '*.c' | sort)

vfsd.wasm: service_entry.c $(VFSD_SOURCES) $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/vfsd"' \
		-DEWOK_WASM_MODULE_BASE=0x15000000 \
		-DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=353370112,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

SD_SOURCES = ../../system/basic/libs/sd/src/partition.c \
	../../system/basic/libs/sd/src/sd.c
EXT_SOURCES = ../../system/basic/libs/ext2/src/ext2fs.c \
	../../system/basic/libs/ext3/src/ext3fs.c \
	../../system/basic/libs/ext3/src/jbd.c

sdfsd.wasm: service_entry.c bsp_sd.c \
		../../system/basic/sys/rootfs/sdfsd.c $(SD_SOURCES) $(EXT_SOURCES) \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/basic/libs/sd/include \
		-I../../system/basic/libs/ext2/include \
		-I../../system/basic/libs/ext3/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/sdfsd"' \
		-DEWOK_WASM_MODULE_BASE=0x16000000 \
		-DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=370147328,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

sessiond.wasm: service_entry.c ../../system/basic/sbin/sessiond/sessiond.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/sessiond"' \
		-DEWOK_WASM_MODULE_BASE=0x15800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=361758720,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

rootfs_probe.wasm: service_entry.c rootfs_probe.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/rootfs_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x12c00000 \
		-DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=315621376,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

ttyd.wasm: service_entry.c ttyd.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/ttyd"' \
		-DEWOK_WASM_MODULE_BASE=0x12000000 \
		-DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=303038464,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

logd.wasm: service_entry.c ../../system/basic/drivers/logd/logd.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/logd -k /dev/log"' \
		-DEWOK_WASM_MODULE_BASE=0x08c00000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=147849216,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

soundd.wasm: service_entry.c soundd.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/soundd /dev/sound0"' \
		-DEWOK_WASM_MODULE_BASE=0x09400000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=156237824,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

sound_probe.wasm: program_entry.c sound_probe.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/sound_probe"' \
		-DEWOK_WASM_ARG0='"sound_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x09c00000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=164626432,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

ethd.wasm: service_entry.c ethd.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/ethd /dev/eth0"' \
		-DEWOK_WASM_MODULE_BASE=0x0d800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=227540992,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

NETD_SOURCES = $(shell find ../../system/network/drivers/netd -name '*.c' | sort)
NETD_EWOKSYS_SOURCES = $(filter-out %/queue.c,$(EWOKSYS_SOURCES))
SOCKET_INET_SOURCES = ../../system/network/libs/socket/src/inet_ntoa.c \
	../../system/network/libs/socket/src/inet_ntop.c \
	../../system/network/libs/socket/src/inet_pton.c

netd.wasm: service_entry.c $(NETD_SOURCES) $(SOCKET_INET_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(NETD_EWOKSYS_SOURCES)
	$(CC) -I../../system/network/drivers/netd \
		-I../../system/network/libs/socket/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/netd /dev/net0"' \
		-DEWOK_WASM_MODULE_BASE=0x0a000000 -DEWOK_WASM_MODULE_SIZE=0x01800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=168820736,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=524288 -o $@

network_probe.wasm: service_entry.c network_probe.c \
		../../system/network/libs/socket/src/socket.c $(SOCKET_INET_SOURCES) $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/network/libs/socket/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/network_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x0b800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 \
		$^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=193986560,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_network_probe_result \
		-Wl,-z,stack-size=131072 -o $@

timed.wasm: service_entry.c ../../system/network/drivers/timed/timed.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/network/libs/ntpc/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/timed /dev/time"' \
		-DEWOK_WASM_MODULE_BASE=0x0c000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=202375168,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

time_probe.wasm: program_entry.c time_probe.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/time_probe"' -DEWOK_WASM_ARG0='"time_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x0c800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=210763776,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_time_probe_result -Wl,-z,stack-size=131072 -o $@

powerd.wasm: service_entry.c ../../machine.virt/system/drivers/powerd/powerd.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/powerd /dev/power0"' \
		-DEWOK_WASM_MODULE_BASE=0x0cc00000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=214958080,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

power_probe.wasm: program_entry.c power_probe.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/power_probe"' -DEWOK_WASM_ARG0='"power_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x0d400000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=223346688,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_power_probe_result -Wl,-z,stack-size=131072 -o $@

persistence_probe.wasm: program_entry.c persistence_probe.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/persistence_probe"' \
		-DEWOK_WASM_ARG0='"persistence_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x0e000000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=235929600,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_persistence_probe_result -Wl,-z,stack-size=131072 -o $@

png_probe.wasm: program_entry.c setjmp_wasm.c png_probe.c \
		../../system/gui/libs/graph/src/graph.c \
		../../system/gui/libs/graph/src/graph_png.c \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/gui/libs/graph/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/png_probe"' -DEWOK_WASM_ARG0='"png_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x0e400000 -DEWOK_WASM_MODULE_SIZE=0x00800000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=240123904,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_png_probe_result -Wl,-z,stack-size=262144 -o $@

nulld.wasm: service_entry.c ../../system/basic/drivers/nulld/nulld.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/nulld /dev/null"' \
		-DEWOK_WASM_MODULE_BASE=0x17800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=395313152,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

ramfsd.wasm: service_entry.c ../../system/basic/drivers/ramfsd/ramfsd.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/ramfsd /tmp"' \
		-DEWOK_WASM_MODULE_BASE=0x18000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=403701760,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

piped.wasm: service_entry.c ../../system/basic/drivers/piped/piped.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/piped /dev/pipe0"' \
		-DEWOK_WASM_MODULE_BASE=0x18800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=412090368,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

timerd.wasm: service_entry.c ../../system/basic/drivers/timerd/timerd.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/timerd"' \
		-DEWOK_WASM_MODULE_BASE=0x19000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=420478976,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

g2dd.wasm: service_entry.c ../../system/gui/drivers/g2dd/g2dd.c \
		../../machine.virt/system/libs/bsp/src/bsp_g2d.c \
		../../system/platform/x86/src/g2d_arch.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../machine.virt/system/libs/bsp/include \
		-I../../system/platform/x86/include/aarch64 \
		-I../../system/gui/libs/g2dclient/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/g2dd /dev/g2d"' \
		-DEWOK_WASM_MODULE_BASE=0x0ec00000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=248512512,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

g2d_probe.wasm: program_entry.c g2d_probe.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/gui/libs/g2dclient/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/g2d_probe"' -DEWOK_WASM_ARG0='"g2d_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x0f400000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=256901120,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_g2d_probe_result -Wl,-z,stack-size=131072 -o $@

displaymand.wasm: service_entry.c \
		../../system/gui/drivers/displaymand/displaymand.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-I../../system/gui/libs/displayman/include \
		-DEWOK_WASM_COMMAND='"/drivers/displaymand"' \
		-DEWOK_WASM_MODULE_BASE=0x1a000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=438304768,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

fbdisplayd.wasm: service_entry.c fbdisplayd.c \
		../../system/gui/libs/displayd/src/displayd.c \
		../../system/gui/libs/displayman/src/displayman.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(GRAPH_SOURCES) \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/gui/libs/display/include \
		-I../../system/gui/libs/displayd/include \
		-I../../system/gui/libs/displayman/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/fbdisplayd /dev/disp0"' \
		-DEWOK_WASM_MODULE_BASE=0x1a800000 -DEWOK_WASM_MODULE_SIZE=0x01000000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=445644800,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_display_resize \
		-Wl,-z,stack-size=262144 -o $@

display_probe.wasm: program_entry.c display_probe.c \
		../../system/gui/libs/display/src/display.c \
		../../system/gui/libs/graph/src/graph.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/gui/libs/display/include \
		-I../../system/gui/libs/graph/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/display_probe"' \
		-DEWOK_WASM_ARG0='"display_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x1b800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=462422016,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

keybd.wasm: service_entry.c keybd.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/keybd /dev/keyb0"' \
		-DEWOK_WASM_MODULE_BASE=0x1c000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=470810624,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

moused.wasm: service_entry.c moused.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/wasm/moused /dev/mouse0"' \
		-DEWOK_WASM_MODULE_BASE=0x1c800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=479199232,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

input_probe.wasm: service_entry.c input_probe.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/input_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x1d000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=487587840,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

FREETYPE_SOURCES = ../../system/gui/libs/freetype/src/base/ftbase.c \
	../../system/gui/libs/freetype/src/base/ftbitmap.c \
	../../system/gui/libs/freetype/src/base/ftdebug.c \
	../../system/gui/libs/freetype/src/base/ftglyph.c \
	../../system/gui/libs/freetype/src/base/ftinit.c \
	../../system/gui/libs/freetype/src/base/ftmm.c \
	../../system/gui/libs/freetype/src/base/ftsystem.c \
	../../system/gui/libs/freetype/src/gzip/ftgzip.c \
	../../system/gui/libs/freetype/src/truetype/truetype.c \
	../../system/gui/libs/freetype/src/sfnt/sfnt.c \
	../../system/gui/libs/freetype/src/smooth/smooth.c

fontd.wasm: service_entry.c setjmp_wasm.c ../../system/gui/drivers/fontd/fontd.c \
		$(FREETYPE_SOURCES) ../../system/basic/libs/tinyjson/src/tinyjson.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/freetype \
		-I. \
		-I../../system/basic/libs/tinyjson/include \
		-DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H='<ftmodule_wasm.h>' \
		-fno-builtin-setjmp -fno-builtin-longjmp \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/fontd"' \
		-DEWOK_WASM_MODULE_BASE=0x1d800000 -DEWOK_WASM_MODULE_SIZE=0x01000000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=495976448,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

font_probe.wasm: program_entry.c font_probe.c \
		../../system/gui/libs/font/src/font_cs.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/font_probe"' \
		-DEWOK_WASM_ARG0='"font_probe"' \
		-DEWOK_WASM_MODULE_BASE=0x1e800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=512753664,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

splashd.wasm: service_entry.c setjmp_wasm.c ../../system/gui/sbin/splashd/splashd.c \
		../../system/gui/libs/display/src/display.c \
		../../system/gui/libs/displayman/src/displayman.c \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/gui/libs/display/include \
		-I../../system/gui/libs/displayman/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/splashd"' \
		-DEWOK_WASM_MODULE_BASE=0x08000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=135266304,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

splash.wasm: program_entry.c ../../system/gui/bin/splash/splash.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/splash -p 100 -m ready"' \
		-DEWOK_WASM_ARG0='"splash"' -DEWOK_WASM_ARG1='"-p"' \
		-DEWOK_WASM_ARG2='"100"' -DEWOK_WASM_ARG3='"-m"' \
		-DEWOK_WASM_ARG4='"EwokOS WebAssembly desktop ready"' \
		-DEWOK_WASM_ARG5='"-i"' \
		-DEWOK_WASM_ARG6='"/usr/system/images/logos/ewokos.png"' \
		-DEWOK_WASM_MODULE_BASE=0x08800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=143654912,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

XSERVER_SOURCES = $(shell find ../../system/xwin/drivers/xserverd -maxdepth 1 -name '*.c' | sort)

xserverd.wasm: service_entry.c setjmp_wasm.c $(XSERVER_SOURCES) \
		../../system/gui/libs/display/src/display.c \
		../../system/gui/libs/displayman/src/displayman.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/drivers/xserverd \
		-I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/display/include \
		-I../../system/gui/libs/displayman/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/drivers/xserverd /dev/x"' \
		-DEWOK_WASM_MODULE_BASE=0x1ec00000 -DEWOK_WASM_MODULE_SIZE=0x01000000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=516947968,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=524288 -o $@

xwm_ewok.wasm: service_entry.c setjmp_wasm.c xwm_ewok.c \
		../../system/xwin/libs/x/src/xwm.c \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/x/xwm_ewok"' \
		-DEWOK_WASM_MODULE_BASE=0x0f800000 -DEWOK_WASM_MODULE_SIZE=0x01000000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=260046848,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xmouse.wasm: service_entry.c ../../system/xwin/sbin/xmouse/xmouse.c \
		../../system/gui/libs/mouse/src/mouse.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/mouse/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/x/xmouse /dev/mouse0"' \
		-DEWOK_WASM_MODULE_BASE=0x1fc00000 -DEWOK_WASM_MODULE_SIZE=0x00100000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=532676608,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=65536 -o $@

xim_none.wasm: service_entry.c xim_none.c \
		../../system/gui/libs/keyb/src/keyb.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/gui/libs/keyb/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/sbin/x/xim_none /dev/keyb0"' \
		-DEWOK_WASM_MODULE_BASE=0x16800000 -DEWOK_WASM_MODULE_SIZE=0x00500000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=378535936,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

X_CLIENT_SOURCES = ../../system/xwin/libs/x/src/x.c \
	../../system/xwin/libs/x/src/xwin.c ../../system/xwin/libs/x/src/xwm.c
X_CLIENT_GRAPH_SOURCES = $(filter-out %/graph_g2d.c,$(GRAPH_SOURCES))

xdemo.wasm: service_entry.c ../../system/xwin/apps/xDemo/xDemo.c \
		$(X_CLIENT_SOURCES) ../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xDemo/xDemo"' \
		-DEWOK_WASM_MODULE_BASE=0x1fd00000 -DEWOK_WASM_MODULE_SIZE=0x002ff000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=533725184,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

xdesktop.wasm: service_entry.c setjmp_wasm.c xdesktop.c \
		$(X_CLIENT_SOURCES) ../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/x/xlauncher"' \
		-DEWOK_WASM_MODULE_BASE=0x13800000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=328204288,$(SERVICE_LDFLAGS)) \
		-Wl,--export-if-defined=ewok_xdesktop_click_count \
		-Wl,-z,stack-size=262144 -o $@

xcores.wasm: service_entry.c xcores.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xcores/xcores"' \
		-DEWOK_WASM_MODULE_BASE=0x12800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=311427072,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

xapps.wasm: service_entry.c setjmp_wasm.c xapps.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xapps/xapps"' \
		-DEWOK_WASM_MODULE_BASE=0x11800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=294653952,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xfinder.wasm: service_entry.c setjmp_wasm.c xfinder.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xfinder/xfinder"' \
		-DEWOK_WASM_MODULE_BASE=0x11c00000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=298844160,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

ximg.wasm: service_entry.c setjmp_wasm.c ximg.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/ximg/ximg"' \
		-DEWOK_WASM_MODULE_BASE=0x13000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=319815680,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xread.wasm: service_entry.c xread.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xread/xread"' \
		-DEWOK_WASM_MODULE_BASE=0x12400000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=307232768,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xg2dtest.wasm: service_entry.c xg2dtest.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xg2dtest/xg2dtest"' \
		-DEWOK_WASM_MODULE_BASE=0x19c00000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=433061888,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

wdemo.wasm: service_entry.c setjmp_wasm.c wdemo.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(PNG_GRAPH_SOURCES) \
		$(PNG_SOURCES) $(ZLIB_READ_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(PNG_CFLAGS) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include -I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include -I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include -I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/wDemo/wDemo"' \
		-DEWOK_WASM_MODULE_BASE=0x20000000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=536870912,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xfonts.wasm: service_entry.c xfonts.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c ../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include -I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include -I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include -I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xfonts/xfonts"' \
		-DEWOK_WASM_MODULE_BASE=0x20400000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=541065216,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xtheme.wasm: service_entry.c xtheme.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c ../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include -I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include -I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include -I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xtheme/xtheme"' \
		-DEWOK_WASM_MODULE_BASE=0x20800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=545259520,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xwifi.wasm: service_entry.c xwifi.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c ../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include -I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include -I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include -I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xwifi/xwifi"' \
		-DEWOK_WASM_MODULE_BASE=0x20c00000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=549453824,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

sndplayer.wasm: service_entry.c sndplayer.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c ../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/gui/libs/libogg/bitwise.c ../../system/gui/libs/libogg/framing.c \
		../../system/gui/libs/libvorbis/lib/mdct.c ../../system/gui/libs/libvorbis/lib/smallft.c \
		../../system/gui/libs/libvorbis/lib/block.c ../../system/gui/libs/libvorbis/lib/window.c \
		../../system/gui/libs/libvorbis/lib/lsp.c ../../system/gui/libs/libvorbis/lib/lpc.c \
		../../system/gui/libs/libvorbis/lib/synthesis.c ../../system/gui/libs/libvorbis/lib/info.c \
		../../system/gui/libs/libvorbis/lib/floor1.c ../../system/gui/libs/libvorbis/lib/floor0.c \
		../../system/gui/libs/libvorbis/lib/res0.c ../../system/gui/libs/libvorbis/lib/mapping0.c \
		../../system/gui/libs/libvorbis/lib/registry.c ../../system/gui/libs/libvorbis/lib/codebook.c \
		../../system/gui/libs/libvorbis/lib/sharedbook.c ../../system/gui/libs/libvorbis/lib/lookup.c \
		../../system/gui/libs/libvorbis/lib/analysis.c ../../system/gui/libs/libvorbis/lib/bitrate.c \
		../../system/gui/libs/libvorbis/lib/envelope.c ../../system/gui/libs/libvorbis/lib/psy.c \
		../../system/gui/libs/libvorbis/lib/vorbisfile.c \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include -I../../system/gui/libs/minimp3/include \
		-DHAVE_CONFIG_H -I../../system/gui/libs/libogg -I../../system/gui/libs/libogg/include \
		-I../../system/gui/libs/libvorbis -I../../system/gui/libs/libvorbis/include \
		-I../../system/gui/libs/libvorbis/lib \
		-I../../system/gui/libs/font/include -I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include -I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include -I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-ffunction-sections -fdata-sections \
		-DEWOK_WASM_COMMAND='"/apps/SndPlayer/SndPlayer"' \
		-DEWOK_WASM_MODULE_BASE=0x21000000 -DEWOK_WASM_MODULE_SIZE=0x00800000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=553648128,$(SERVICE_LDFLAGS)) \
		-Wl,--gc-sections -Wl,-z,stack-size=262144 -o $@

clock.wasm: service_entry.c clock.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/clock/clock"' \
		-DEWOK_WASM_MODULE_BASE=0x14800000 -DEWOK_WASM_MODULE_SIZE=0x00200000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=344981504,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

xlog.wasm: service_entry.c xlog.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xlog/xlog"' \
		-DEWOK_WASM_MODULE_BASE=0x14a00000 -DEWOK_WASM_MODULE_SIZE=0x00300000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=347078656,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

xprocs.wasm: service_entry.c xprocs.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xprocs/xprocs"' \
		-DEWOK_WASM_MODULE_BASE=0x14d00000 -DEWOK_WASM_MODULE_SIZE=0x00300000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=350224384,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

xwm_theme.wasm: service_entry.c setjmp_wasm.c xwm_theme.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xwm_theme/xwm_theme"' \
		-DEWOK_WASM_MODULE_BASE=0x11400000 -DEWOK_WASM_MODULE_SIZE=0x00400000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=290455552,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

xterm.wasm: service_entry.c xterm.c $(X_CLIENT_SOURCES) \
		../../system/gui/libs/gterminal/src/gterminal.c \
		../../system/gui/libs/textgrid/src/textgrid.c \
		../../system/gui/libs/font/src/font.c \
		../../system/gui/libs/font/src/font_cs.c \
		../../system/gui/libs/g2dclient/src/g2dclient.c $(GRAPH_SOURCES) \
		../../system/basic/libs/tinyjson/src/tinyjson.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) -I../../system/xwin/libs/x/include \
		-I../../system/gui/libs/gterminal/include \
		-I../../system/gui/libs/textgrid/include \
		-I../../system/gui/libs/font/include \
		-I../../system/gui/libs/freetype/include \
		-I../../system/gui/libs/graph/include \
		-I../../system/gui/libs/g2dclient/include \
		-I../../system/gui/libs/mouse/include \
		-I../../system/basic/libs/tinyjson/include \
		$(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/apps/xterm/xterm"' \
		-DEWOK_WASM_MODULE_BASE=0x10800000 -DEWOK_WASM_MODULE_SIZE=0x00100000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=276889600,$(SERVICE_LDFLAGS)) \
		-Wl,--export=ewok_xterm_write \
		-Wl,--export=ewok_xterm_input_count \
		-Wl,-z,stack-size=131072 -o $@

cat.wasm: program_entry.c ../../system/basic/bin/cat/cat.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/cat /etc/init.rd"' \
		-DEWOK_WASM_ARG0='"cat"' -DEWOK_WASM_ARG1='"/etc/init.rd"' \
		-DEWOK_WASM_MODULE_BASE=0x11000000 \
		-DEWOK_WASM_MODULE_SIZE=0x00400000 -Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=286261248,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

uname.wasm: program_entry.c ../../system/basic/bin/uname/uname.c $(SYSCALL_SRC) \
		../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-DEWOK_WASM_COMMAND='"/bin/uname -a"' \
		-DEWOK_WASM_ARG0='"uname"' -DEWOK_WASM_ARG1='"-a"' \
		-DEWOK_WASM_MODULE_BASE=0x19800000 -DEWOK_WASM_MODULE_SIZE=0x00400000 \
		-Dmain=ewok_program_main $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=428867584,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=131072 -o $@

shell.wasm: service_entry.c ../../system/basic/bin/shell/shell.c \
		../../system/basic/bin/shell/gets.c ../../system/basic/bin/shell/cmd.c \
		$(SYSCALL_SRC) ../../system/basic/libc/libgloss/compat.c \
		../../system/basic/libc/libgloss/syscalls.c $(EWOKSYS_SOURCES)
	$(CC) $(filter-out -DEWOK_WASM_MODULE_BASE=% -DEWOK_WASM_MODULE_SIZE=%,$(SERVICE_CFLAGS)) \
		-I../../system/basic/bin/shell -I../../system/network/libs/utils/include \
		-DEWOK_WASM_COMMAND='"/bin/shell"' \
		-DEWOK_WASM_MODULE_BASE=0x10900000 \
		-DEWOK_WASM_MODULE_SIZE=0x00700000 $^ \
		$(subst --global-base=$(GLOBAL_BASE),--global-base=278921216,$(SERVICE_LDFLAGS)) \
		-Wl,-z,stack-size=262144 -o $@

clean:
	rm -f init.wasm worker.wasm core.wasm vfsd.wasm sdfsd.wasm sessiond.wasm rootfs_probe.wasm \
	ttyd.wasm logd.wasm soundd.wasm sound_probe.wasm ethd.wasm netd.wasm network_probe.wasm \
		timed.wasm time_probe.wasm powerd.wasm power_probe.wasm \
		persistence_probe.wasm png_probe.wasm \
		nulld.wasm ramfsd.wasm piped.wasm timerd.wasm g2dd.wasm g2d_probe.wasm cat.wasm \
		displaymand.wasm fbdisplayd.wasm keybd.wasm moused.wasm \
		display_probe.wasm input_probe.wasm fontd.wasm font_probe.wasm \
		splashd.wasm splash.wasm xserverd.wasm xwm_ewok.wasm xmouse.wasm \
		xim_none.wasm xdesktop.wasm xdemo.wasm xcores.wasm xapps.wasm \
		xfinder.wasm ximg.wasm xread.wasm xg2dtest.wasm wdemo.wasm xfonts.wasm xtheme.wasm \
		xwifi.wasm sndplayer.wasm clock.wasm xlog.wasm xprocs.wasm \
		xwm_theme.wasm xterm.wasm uname.wasm shell.wasm
