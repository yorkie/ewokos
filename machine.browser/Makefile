.PHONY: all clean test serve rootfs

ROOTFS_SOURCE ?= ../machine.virt/system/build_aarch64
MKE2FS ?= $(if $(wildcard /opt/homebrew/opt/e2fsprogs/sbin/mke2fs),/opt/homebrew/opt/e2fsprogs/sbin/mke2fs,mke2fs)

all:
	$(MAKE) -C kernel
	$(MAKE) -C user
	$(MAKE) rootfs.img

rootfs: rootfs.img

rootfs.img: rootfs-overlay/etc/init.wasm.rd
	@test -d "$(ROOTFS_SOURCE)" || { \
		echo "missing EwokOS rootfs source: $(ROOTFS_SOURCE)"; exit 1; \
	}
	@stage=$$(mktemp -d "$${TMPDIR:-/tmp}/ewok-wasm-rootfs.XXXXXX"); \
	trap 'rm -rf "$$stage"' EXIT; \
	cp -R "$(ROOTFS_SOURCE)/." "$$stage/"; \
	cp -R rootfs-overlay/. "$$stage/"; \
	truncate -s 160M "$@"; \
	"$(MKE2FS)" -q -F -t ext3 -O '^64bit,^metadata_csum' \
		-L rootfs -b 4096 -I 128 -d "$$stage" "$@"

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C user clean

test: all
	node test-kernel.mjs

serve: all
	python3 serve.py 8765
