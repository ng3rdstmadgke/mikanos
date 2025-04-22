```bash
sudo apt update
sudo apt install -y make clang lld qemu-system-x86
```

qemuでOSを実行

```bash
cd os

# ビルド
make build

# イメージ作成
make image

# qemuでイメージを実行 (CLI)
make run

# qemuでイメージを実行 (GUI)
make run-graphic

# お掃除
make clean
```