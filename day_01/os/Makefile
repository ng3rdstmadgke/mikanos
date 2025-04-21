
# -target x86_64-pc-win32-coff: 出力を Windows向けの x86_64 COFF形式（＝UEFIで使える形式）にする
# -mno-red-zone: red zone（スタックの下の128バイト）を使わない。割り込みが入る環境（UEFIなど）では red zone は危険なのでオフにする
# -fno-stack-protector: スタックプロテクションをオフにする。UEFIでは実行時ライブラリがないので不要（むしろ動かない）
# -fshort-wchar: wchar_t を 2バイト（UTF-16）にする。UEFI は CHAR16 = wchar_t（16bit）と前提しているため
# -Wall: 全ての一般的な警告を有効化（コードの品質チェック）
# -o $@ 出力ファイル名を指定
# -c: コンパイルのみ実行（リンクはしない）
%.o: %.c
	clang -target x86_64-pc-win32-coff \
	  -mno-red-zone \
	  -fno-stack-protector \
	  -fshort-wchar \
	  -Wall \
	  -o $@ -c $<

# -subsystem:efi_application: UEFIファームウェアで起動可能な .efi 形式にする
# -entry:EfiMain: エントリーポイントを EfiMain にする
# -out:$@: 出力ファイル名を指定
%.efi: %.o
	lld-link /subsystem:efi_application /entry:EfiMain /out:$@ $<

disk.img: hello.efi
	qemu-img create -f raw disk.img 200M
	mkfs.fat -n "MIKAN OS" -s 2 -f 2 -R 32 -F 32 $@
	sudo parted $@ print
	mkdir -p mnt
	sudo mount -o loop $@ mnt
	sudo mkdir -p mnt/EFI/BOOT
	sudo cp $< mnt/EFI/BOOT/BOOTX64.EFI
	sudo umount mnt

.PHONY: build
build: hello.efi

.PHONY: image
image: disk.img

.PHONY: run
run: image
	sudo qemu-system-x86_64 \
  -drive if=pflash,file=OVMF_CODE.fd,format=raw \
  -drive if=pflash,file=OVMF_VARS.fd,format=raw \
  -drive file=disk.img,format=raw \
  -nographic

.PHONY: clean
clean:
	rm -rf hello.efi hello.o disk.img mnt
