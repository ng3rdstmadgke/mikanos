# 環境構築整備


```bash
sudo apt update
sudo apt install -y ansible
ansible-playbook -K -i ansible/inventory ansible/provision.yml
```

# Qemuで実行

```bash
# OSイメージをQEMUで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用) 
make run

# OSイメージをQEMUのノーグラフィックモードで起動します (OVMF_CODE_4M.fd, OVMF_VARS_4M.fdを利用)
make run-nographic
```

# 実機で実行

```bash
# MikanLoaderPkg を edk2 でビルドします
make build-edk2

# 接続されているディスクを確認
sudo fdisk -l
# ...
# ディスク /dev/sda: 28.91 GiB, 31042043904 バイト, 60628992 セクタ
# Disk model: USB Flash Disk  
# 単位: セクタ (1 * 512 = 512 バイト)
# セクタサイズ (論理 / 物理): 512 バイト / 512 バイト
# I/O サイズ (最小 / 推奨): 512 バイト / 512 バイト
# ディスクラベルのタイプ: gpt
# ディスク識別子: 1B5CA5F7-2AE4-4ACE-B9AC-15FC7FE1E041

DEVICE_FILE=/dev/sda

sudo parted $DEVICE_FILE

# パーティションテーブルを作成
(parted) mklabel gpt
(parted) print
# モデル: BUFFALO USB Flash Disk (scsi)
# ディスク /dev/sda: 31.0GB
# セクタサイズ (論理/物理): 512B/512B
# パーティションテーブル: gpt
# ディスクフラグ:
# 
# 番号  開始  終了  サイズ  ファイルシステム  名前  フラグ

# EFIシステムパーティション(ESP)を作成
(parted) mkpart primary fat32 1MiB 1GiB
(parted) set 1 esp on
(parted) print
# モデル: BUFFALO USB Flash Disk (scsi)
# ディスク /dev/sda: 31.0GB
# セクタサイズ (論理/物理): 512B/512B
# パーティションテーブル: gpt
# ディスクフラグ:
# 
# 番号  開始    終了    サイズ  ファイルシステム  名前     フラグ
#  1    1049kB  1074MB  1073MB                    primary  boot, esp

# partedを終了
(parted) quit


# パーティションが作成できていることを確認
sudo fdisk -l $DEVICE_FILE
# ディスク /dev/sda: 28.91 GiB, 31042043904 バイト, 60628992 セクタ
# Disk model: USB Flash Disk
# 単位: セクタ (1 * 512 = 512 バイト)
# セクタサイズ (論理 / 物理): 512 バイト / 512 バイト
# I/O サイズ (最小 / 推奨): 512 バイト / 512 バイト
# ディスクラベルのタイプ: gpt
# ディスク識別子: 832B57BC-0A3B-442D-BD4B-5597B99950AB
# 
# デバイス   開始位置 最後から  セクタ サイズ タイプ
# /dev/sda1      2048  2097151 2095104  1023M EFI システム

# ファイルシステムを作成
sudo mkfs.vfat -F32 ${DEVICE_FILE}1

# ファイルシステムがfat32になっていることを確認
sudo parted $DEVICE_FILE print
# モデル: BUFFALO USB Flash Disk (scsi)
# ディスク /dev/sda: 31.0GB
# セクタサイズ (論理/物理): 512B/512B
# パーティションテーブル: gpt
# ディスクフラグ:
#
# 番号  開始    終了    サイズ  ファイルシステム  名前     フラグ
#  1    1049kB  1074MB  1073MB  fat32             primary  boot, esp

# /EFI/BOOT/BOOTX64.EFIを配置
sudo mkdir -p /mnt/esp
sudo mount ${DEVICE_FILE}1 /mnt/esp
sudo mkdir -p /mnt/esp/EFI/BOOT
sudo cp build/BOOTX64.EFI /mnt/esp/EFI/BOOT/BOOTX64.EFI
sudo umount /mnt/esp
```
