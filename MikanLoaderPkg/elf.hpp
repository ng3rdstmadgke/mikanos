#pragma once

#include <stdint.h>

// NOTE: uintptr_t, uint64_tなどは stdint.h で定義されている
typedef uintptr_t Elf64_Addr;
typedef uint64_t  Elf64_Off;
typedef uint16_t  Elf64_Half;
typedef uint32_t  Elf64_Word;
typedef int32_t   Elf64_Sword;
typedef uint64_t  Elf64_Xword;
typedef int64_t   Elf64_Sxword;

#define EI_NIDENT 16

// 64bit ELFファイルのヘッダ
typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf64_Half    e_type;
  Elf64_Half    e_machine;
  Elf64_Word    e_version;
  Elf64_Addr    e_entry;
  Elf64_Off     e_phoff;      // プログラムヘッダ(配列)のオフセット
  Elf64_Off     e_shoff;
  Elf64_Word    e_flags;
  Elf64_Half    e_ehsize;
  Elf64_Half    e_phentsize;  // プログラムヘッダ(配列)の要素のサイズ
  Elf64_Half    e_phnum;      // プログラムヘッダ(配列)の要素数
  Elf64_Half    e_shentsize;
  Elf64_Half    e_shnum;
  Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

// 64bit ELFファイルのプログラムヘッダの要素
typedef struct {
  Elf64_Word  p_type;    // PHDR, LOADなどのセグメント種別
  Elf64_Word  p_flags;   // フラグ
  Elf64_Off   p_offset;  // オフセット
  Elf64_Addr  p_vaddr;   // 仮想 Addr
  Elf64_Addr  p_paddr;
  Elf64_Xword p_filesz;  // ファイルサイズ
  Elf64_Xword p_memsz;   // メモリサイズ
  Elf64_Xword p_align;
} Elf64_Phdr;

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7