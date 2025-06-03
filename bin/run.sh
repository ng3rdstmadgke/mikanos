#!/bin/bash

function usage {
cat >&2 <<EOF
qemuでイメージを起動する

[usage]
 $0 [options]

[options]
 -h | --help:
   ヘルプを表示
 -l | --legacy:
   古いOVMFを利用してイメージを起動する
 -n | --nographic:
   ノーグラフィックモードでイメージを起動する
   
EOF
exit 1
}

PROJECT_DIR=$(cd $(dirname $0)/..; pwd)

LEGACY=
NOGRAPHIC=
QEMU_OPTIONS=
while [ "$#" != 0 ]; do
  case $1 in
    -h | --help      ) usage ;;
    -l | --legacy    ) LEGACY=1 ;;
    -n | --nographic ) NOGRAPHIC=1 ;;
    -* | --*         ) echo "$1 : 不正なオプションです" >&2 ;;
    *                ) args+=("$1") ;;
  esac
  shift
done

[ "${#args[@]}" != 0 ] && usage

cd $PROJECT_DIR

if [ ! -f "$PROJECT_DIR/build/disk.img" ]; then
  echo "$PROJECT_DIR/build/disk.img が存在しません" >&2
  exit 1
fi

if [ -n "$LEGACY" ]; then
	cp $PROJECT_DIR/resource/OVMF_VARS.fd $PROJECT_DIR/build/OVMF_VARS.fd
  QEMU_OPTIONS="$QEMU_OPTIONS -drive if=pflash,format=raw,file=$PROJECT_DIR/resource/OVMF_CODE.fd,readonly=on"
  QEMU_OPTIONS="$QEMU_OPTIONS -drive if=pflash,format=raw,file=$PROJECT_DIR/build/OVMF_VARS.fd"
else
	cp /usr/share/OVMF/OVMF_VARS_4M.fd $PROJECT_DIR/build/OVMF_VARS_4M.fd
  QEMU_OPTIONS="$QEMU_OPTIONS -drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_CODE_4M.fd,readonly=on"
  QEMU_OPTIONS="$QEMU_OPTIONS -drive if=pflash,format=raw,file=$PROJECT_DIR/build/OVMF_VARS_4M.fd"
fi

if [ -n "$NOGRAPHIC" ]; then
  QEMU_OPTIONS="$QEMU_OPTIONS -nographic"
else
  QEMU_OPTIONS="$QEMU_OPTIONS -device nec-usb-xhci,id=xhci"
  QEMU_OPTIONS="$QEMU_OPTIONS -device usb-mouse"
  QEMU_OPTIONS="$QEMU_OPTIONS -device usb-kbd"
  QEMU_OPTIONS="$QEMU_OPTIONS -monitor stdio" # ターミナルでQEMUモニタを起動
fi

# -s : -gdb tcp::1234 のショートカット (QEMUをGDBサーバーモードで起動することで)
sudo qemu-system-x86_64 $QEMU_OPTIONS \
  -m 1G \
  -drive if=ide,index=0,media=disk,format=raw,file=$PROJECT_DIR/build/disk.img \
  -s