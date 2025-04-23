# 環境構築

```bash
sudo apt update

# とりあえず必要なパッケージ
sudo apt-get install -y git vim

# edk2のビルドで必要
sudo apt-get install -y make clang lld llvm nasm uuid-dev acpica-tools

# イメージの起動で必要
# - OVMF (Open Virtual Machine Firmware)
#   QEMU などの仮想マシンで UEFI をサポートするためのファームウェアで、EDK IIプロジェクトの一部として提供されています。
sudo apt-get install -y qemu-system-x86 ovmf
```

# 実行

```bash
# ヘルプ
make

# MikanLoaderPkg を edk2 でビルドします
make build-edk2

# OSのイメージファイル(build/disk.img) を作成します
make build-image

# OSイメージをQEMUで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用) 
run

# OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用)
run-nographic

# OSイメージをQEMUで起動します (OVMF_CODE.fd, OVMF_VARS.fd を利用)
run-legacy

# OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE.fd, OVMF_VARS.fd を利用)
run-nographic-legacy
```