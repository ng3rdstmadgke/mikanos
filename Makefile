MIKAN_LOADER_PKG := $(wildcard MikanLoaderPkg/*)

build/BOOTX64.EFI: $(MIKAN_LOADER_PKG)
	bash bin/build-edk2.sh
	mkdir -p build
	cp edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi $@

build/disk.img: build/BOOTX64.EFI
	qemu-img create -f raw build/disk.img 200M
	mkfs.fat -n "MIKAN OS" -s 2 -f 2 -R 32 -F 32 $@
	sudo parted $@ print
	mkdir -p build/mnt
	sudo mount -o loop $@ build/mnt
	sudo mkdir -p build/mnt/EFI/BOOT
	sudo cp $< build/mnt/EFI/BOOT/BOOTX64.EFI
	sudo umount build/mnt
	rmdir build/mnt

.PHONY: build-edk2
build-edk2: build/BOOTX64.EFI  ## MikanLoaderPkg を edk2 でビルドします

.PHONY: build-image
build-image: build/disk.img  ## OSのイメージファイル(build/disk.img) を作成します

.PHONY: run-legacy
run-legacy: build-image  ## OSイメージをQEMUで起動します (OVMF_CODE.fd, OVMF_VARS.fd を利用)
	cp resource/OVMF_VARS.fd build/OVMF_VARS.fd
	sudo qemu-system-x86_64 \
    -m 1G \
		-drive if=pflash,format=raw,file=resource/OVMF_CODE.fd,readonly=on \
		-drive if=pflash,format=raw,file=build/OVMF_VARS.fd \
		-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
    -device nec-usb-xhci,id=xhci \
    -device usb-mouse -device usb-kbd \
    -monitor stdio \

.PHONY: run-nographic-legacy
run-nographic-legacy: build-image  ## OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE.fd, OVMF_VARS.fd を利用)
	cp resource/OVMF_VARS.fd build/OVMF_VARS.fd
	sudo qemu-system-x86_64 \
    -m 1G \
		-drive if=pflash,format=raw,file=resource/OVMF_CODE.fd,readonly=on \
		-drive if=pflash,format=raw,file=build/OVMF_VARS.fd \
		-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
		-nographic

.PHONY: run
run: build-image  ## OSイメージをQEMUで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用)
	cp /usr/share/OVMF/OVMF_VARS_4M.fd build/OVMF_VARS_4M.fd
	sudo qemu-system-x86_64 \
    -m 1G \
		-drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_CODE_4M.fd,readonly=on \
		-drive if=pflash,format=raw,file=build/OVMF_VARS_4M.fd \
		-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
    -device nec-usb-xhci,id=xhci \
    -device usb-mouse -device usb-kbd \
    -monitor stdio \

.PHONY: run-nographic
run-nographic: build-image  ## OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用)
	cp /usr/share/OVMF/OVMF_VARS_4M.fd build/OVMF_VARS_4M.fd
	sudo qemu-system-x86_64 \
    -m 1G \
		-drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_CODE_4M.fd,readonly=on \
		-drive if=pflash,format=raw,file=build/OVMF_VARS_4M.fd \
		-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
		-nographic

.PHONY: clean
clean:	## ビルドしたファイルを削除します
	rm -rf build

.PHONY: help
.DEFAULT_GOAL := help
help: ## HELP表示
	@grep --no-filename -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'