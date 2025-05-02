MIKAN_LOADER_PKG := $(wildcard MikanLoaderPkg/*)

build/BOOTX64.EFI: $(MIKAN_LOADER_PKG)
	bash bin/build-edk2.sh
	mkdir -p build
	cp edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi $@
	chmod +x $@

# -O2 レベル2の最適化を行う
# -Wall 警告をたくさん出す
# -g デバッグ情報付きでコンパイルする
# --target=x86_64-elf x86_64向けの機械語を生成する。出力ファイルの形式をELFとする
# -ffreestanding フリースタンディング環境(OSがない環境)向けにコンパイルする
# -mno-red-zone Red Zone機能を無効にする
# -fno-exceptions C++の例外機能を使わない
# -fno-rtti C++の動的型情報を使わない
# -std=c++17 C++のバージョンをC++17とする
# -c コンパイルのみする。リンクはしない。
# -o build/kernel/main.o 出力先を指定
build/kernel/main.o: kernel/main.cpp
	mkdir -p build/kernel
	clang++ \
	  -O2 \
	  -Wall \
	  -g \
	  --target=x86_64-elf \
	  -ffreestanding \
	  -mno-red-zone \
	  -fno-exceptions \
	  -fno-rtti \
	  -std=c++17 \
	  -c \
	  -o build/kernel/main.o \
	  kernel/main.cpp


# --entry KernelMain KernelMain()をエントリーポイントとする
# -z norelro リロケーション情報を読み込み専用にする機能を使わない
# --image-base 0x100000 出力されたバイナリのベースアドレスを0x100000番地とする
# -o build/kernel/kernel.elf 出力先を指定
# --static 静的リンクを行う
build/kernel/kernel.elf: build/kernel/main.o
	ld.lld \
	--entry KernelMain \
	-z norelro \
	--image-base 0x100000 \
	-static \
	-o build/kernel/kernel.elf \
	build/kernel/main.o


build/disk.img: build/BOOTX64.EFI build/kernel/kernel.elf
	qemu-img create -f raw $@ 200M
	mkfs.fat -n "MIKAN OS" -s 2 -f 2 -R 32 -F 32 $@
	sudo parted $@ print
	mkdir -p build/mnt
	sudo mount -o loop $@ build/mnt
	sudo mkdir -p build/mnt/EFI/BOOT
	sudo cp $< build/mnt/EFI/BOOT/BOOTX64.EFI
	sudo cp build/kernel/kernel.elf build/mnt/kernel.elf
	sudo umount build/mnt
	rmdir build/mnt


.PHONY: build-edk2
build-edk2: build/BOOTX64.EFI  ## MikanLoaderPkg を edk2 でビルドします

.PHONY: build-kernel
build-kernel: build/kernel/kernel.elf  ## kernel/ 配下のソースコードをビルドし、build/kernel/kernel.elf を作成します。

.PHONY: build-image
build-image: build/disk.img  ## OSのイメージファイル(build/disk.img) を作成します


.PHONY: mount-image
mount-image:  ## OSのイメージファイル(build/disk.img) を build/mnt にマウントします
	mkdir -p build/mnt
	sudo mount -o loop build/disk.img build/mnt

.PHONY: umount-image
umount-image:  ## OSのイメージファイル(build/disk.img) をbuild/mnt からアンマウントします
	sudo umount build/mnt

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
		-s

.PHONY: run-nographic-legacy
run-nographic-legacy: build-image  ## OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE.fd, OVMF_VARS.fd を利用)
	cp resource/OVMF_VARS.fd build/OVMF_VARS.fd
	sudo qemu-system-x86_64 \
    -m 1G \
		-drive if=pflash,format=raw,file=resource/OVMF_CODE.fd,readonly=on \
		-drive if=pflash,format=raw,file=build/OVMF_VARS.fd \
		-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
		-s \
		-nographic

# -monitor stdio : ターミナルでQEMUモニタを起動
# -s : -gdb tcp::1234 のショートカット (QEMUをGDBサーバーモードで起動することで)
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
		-s

.PHONY: run-nographic
run-nographic: build-image  ## OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用)
	cp /usr/share/OVMF/OVMF_VARS_4M.fd build/OVMF_VARS_4M.fd
	sudo qemu-system-x86_64 \
    -m 1G \
		-drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_CODE_4M.fd,readonly=on \
		-drive if=pflash,format=raw,file=build/OVMF_VARS_4M.fd \
		-drive if=ide,index=0,media=disk,format=raw,file=build/disk.img \
		-s \
		-nographic

.PHONY: clean
clean:	## ビルドしたファイルを削除します
	rm -rf build

.PHONY: help
.DEFAULT_GOAL := help
help: ## HELP表示
	@grep --no-filename -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'