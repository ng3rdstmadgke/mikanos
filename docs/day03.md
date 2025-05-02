# QEMUモニタを操作する
```bash
make run
```

nographicでやる場合

```bash
make run-nographic
# ctrl-a + c # コンソールとモニタを切り替える
```

## QEMUモニタでレジスタの値を確認

```bash
# CPUの各レジスタの値を表示
(qemu) info registers
# CPU#0
# RAX=0000000000000000 RBX=000000003e5696a0 RCX=000000003ed0eda0 RDX=0000000000000007
# RSI=000000003fe92680 RDI=000000003fe92638 RBP=0000000000000082 RSP=000000003fe925f0
# R8 =000000003e571000 R9 =0000000000001000 R10=000000003de81218 R11=0000000000000000
# R12=000000003fe93fe0 R13=000000003e98c000 R14=000000003de8385a R15=000000003de83d80
# RIP=000000003de82411 RFL=00000206 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0

# RIPレジスタ(次に実行される予定の機械語命令の位置を示す)に格納されているアドレスをメモリダンプ(xコマンド)で確認
# x /fmt addr
#   /fmt は /[個数][フォーマット][サイズ] 形式
#     - 個数 : 何個分表示するか
#     - フォーマット : x=16進数表記, d=10進数表記, i=機械語命令を逆アセンブル
#     - サイズ : 何バイトを1単位として解釈するか (b=1バイト, h=2バイト, w=4バイト, g=8バイト)
(qemu) x /4xb 0x000000003de82411
# 000000003de82411: 0xeb 0xfe 0x48 0x83
```


## GDBでRIPレジスタで指定されているメモリアドレスの機械語命令を逆アセンブル

```bash
gdb


# QEMUのGDBサーバーに接続 (QEMU実行時に-sオプションを指定すると接続できる)
(gdb) target remote localhost:1234
# Remote debugging using :1234
# warning: No executable has been specified and target does not support
# determining executable automatically.  Try using the "file" command.
# 0x000000003de82411 in ?? ()

# RIPレジスタの示すメモリアドレスから4命令を逆アセンブル
(gdb) x /4i 0x000000003de82411
# => 0x3de82411:	jmp    0x3de82411
#    0x3de82413:	sub    $0x28,%rsp
#    0x3de82417:	call   0x3de82240
#    0x3de8241c:	mov    %rdi,%rax

# 現在の命令ポインタから4命令を逆アセンブル
(gdb) x/10i $pc
# => 0x3de82411:	jmp    0x3de82411
#    0x3de82413:	sub    $0x28,%rsp
#    0x3de82417:	call   0x3de82240
#    0x3de8241c:	mov    %rdi,%rax

(gdb) exit
```

`jmp 0x3de82411` は `0x3de82411` にジャンプするという命令ですが、

### GDBでよく使うコマンド

| コマンド | 説明 |
| --- | --- |
| `info registers` | 全レジスタの値を表示 |
| `x/10i $pc` | 現在の命令ポインタから10命令を逆アセンブル表示 |
| `break <関数名>` | 指定した関数にブレークポイントを設定 |
| `continue` | 実行を再開 |
| `step` / `next` | ステップ実行（関数内に入る/入らない） |
| `bt` | スタックトレースを表示 |
| `quit` | GDBを終了 |