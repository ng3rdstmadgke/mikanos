#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "elf.hpp"

// UINT系: https://github.com/tianocore/edk2/blob/edk2-stable202302/EmbeddedPkg/Include/libfdt_env.h#L19
// UINT8 uint8_t
// UINT16 uint16_t
// UINT32 uint32_t
// UINT64 uint64_t
// UINTN uintptr_t CPUアーキテクチャ依存の符号なし整数型。32bitでは4byte , 64bitでは(8byte)

// メモリマップの構造体
struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

/**
 * メモリマップを取得する関数
 */
EFI_STATUS GetMemoryMap(struct MemoryMap *map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  // gBS(EFI_BOOT_SERVICES): https://github.com/tianocore/edk2/blob/edk2-stable202208/MdePkg/Include/Uefi/UefiSpec.h#L1863
  //   OSを起動するために必要な機能を提供するブートサービスを表すグローバル変数
  //
  // EFI_GET_MEMORY_MAP: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L239
  //   呼び出し時点のメモリマップを取得し、MemoryMap（第二引数)で指定されたメモリ領域に書き込みます。
  //   正常にメモリマップが取得できると EFI_SUCCESS を返却。メモリ領域が小さくてメモリマップを書き込めない場合は EFI_BUFFER_TOO_SMALL を返却。
  //
  // EFI_MEMORY_DESCRIPTOR: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L128
  //   メモリディスクリプタはメモリマップの個々のエントリ
  //   UINT32                Type           メモリ領域の種別
  //   EFI_PHYSICAL_ADDRESS  PhysicalStart  メモリ領域先頭の物理メモリアドレス
  //   EFI_VIRTUAL_ADDRESS   VirtualStart   メモリ領域先頭の仮想メモリアドレス
  //   UINT64                NumberOfPages  メモリ領域の大きさ (4KiBページ単位)
  //   UINT64                Attribute      メモリ領域が使える用途を示すビット集合
  return gBS->GetMemoryMap(
    &map->map_size,                       // IN OUT UINTN                *MemoryMapSize メモリマップ書き込み用のメモリ領域のサイズ(バイト)
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,  // OUT  EFI_MEMORY_DESCRIPTOR  *MemoryMap メモリマップ書込み用メモリ領域の先頭ポインタ(メモリディスクリプタの配列)
    &map->map_key,                        // OUT  UINTN                  *MapKey  メモリマップのハッシュ値的なもの (これに変化がなければメモリマップに変化はない)
    &map->descriptor_size,                // OUT  UINTN                  *DescriptorSize メモリディスクリプタ のサイズ
    &map->descriptor_version              // OUT  UINT32                 *DescriptorVersion メモリディスクリプタのバージョン番号
  );
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}


/**
 * メモリマップをCSV形式でファイルに保存する
 */
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file) {
  CHAR8 buf[256];
  // UINTN: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/X64/ProcessorBind.h#L229
  // 32-bitでは4byte, 64-bitでは8byteの符号なし整数
  UINTN len;

  CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Library/BaseLib.h#L1673
  len = AsciiStrLen(header);
  // EFI_FILE_WRITE: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L220
  file->Write(
    file,  // IN EFI_FILE_PROTOCOL  *This,
    &len,  // IN OUT UINTN          *BufferSize,
    header // IN VOID               *Buffer
  );

  // %08lx : unsigned longの16進数をゼロ埋め8桁で表示 (例: 0000abcd)
  Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

  // EFI_PHYSICAL_ADDRESS: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiBaseType.h#L50
  EFI_PHYSICAL_ADDRESS iter;
  int i;
  // メモリマップからメモリディスクリプタ(構造体)をいてレートして、ファイルに書き込む
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++
  ) {
    // 整数型のiterをEFI_MEMORY_DESCRIPTOR* (ポインタ型) にキャスト(型変換)している
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    // AsciiSPrint: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Library/PrintLib.h#L677
    //   char配列に整形した文字列を書き込む (sprintf()とほぼ同じ)
    len = AsciiSPrint(
      buf,
      sizeof(buf),
      // %u : unsigned int の10進数(符号なし整数)を表示
      // %x : unsigned int の16進数(小文字)を表示
      // %-ls : wchar_t* のワイド文字列を左寄せで表示 (l は wchar_t* を意味しています)
      // %08lx : unsigned longの16進数をゼロ埋め8桁で表示 (例: 0000abcd)
      // %lx : unsigned longの16進数を表示
      "%u, %x, %-ls, %08lx, %lx, %lx\n",
      i,
      desc->Type,
      GetMemoryTypeUnicode(desc->Type),
      desc->PhysicalStart,
      desc->NumberOfPages,
      desc->Attribute & 0xffffflu
    );
    file->Write(file, &len, buf);
  }
  return EFI_SUCCESS;
}

/**
 * 書き込み先のファイルを開く
 */
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L73
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  // EFI_OPEN_PROTOCOL: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L1330
  //   UEFI対応のファイルシステム上でファイルやディレクトリに対する入出力操作を行うためのインターフェース
  // EFI_GUID: https://github.com/tianocore/edk2/blob/edk2-stable202208/MdePkg/Include/Uefi/UefiBaseType.h#L24
  //           https://github.com/tianocore/edk2/blob/edk2-stable202208/BaseTools/Source/C/Include/Common/BaseTypes.h#L127
  //   UEFI環境で「何かを一意に識別するための型」であり、主にプロトコルやサービスの個別識別、データ構造の区別などの用途で使われます。
  gBS->OpenProtocol(
    image_handle,                        // IN  EFI_HANDLE  Handle,               オープンされているプロトコルインタフェースのハンドル
    &gEfiLoadedImageProtocolGuid,        // IN  EFI_GUID    *Protocol             プロトコルのGUID(識別子)
    (VOID**)&loaded_image,               // OUT VOID        **Interface  OPTIONAL 対応するプロトコルインタフェースへのポインタが返されるアドレス
    image_handle,                        // IN  EFI_HANDLE  AgentHandle           Protocol と Interface で指定されたプロトコルとインターフェースを開いているエージェントのハンドル。
    NULL,                                // IN  EFI_HANDLE  ControllerHandle      エージェントがUEFIドライバモデルに従うドライバの場合はプロトコルインターフェイスを必要とするコントローラハンドル、そうでなければNULL
    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL // IN  UINT32      Attributes            Handle と Protocol で指定されたプロトコルインタフェースのオープンモード。
  );

  gBS->OpenProtocol(
    loaded_image->DeviceHandle,
    &gEfiSimpleFileSystemProtocolGuid,
    (VOID**)&fs,
    image_handle,
    NULL,
    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
  );

  // EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L59
  //   EFI_SIMPLE_FILE_SYSTEM_PROTOCOL 構造体のメンバー関数
  //   ファイルシステムボリュームのルートディレクトリを開く
  fs->OpenVolume(
    fs,  // IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
    root // OUT EFI_FILE_PROTOCOL                **Root
  );

  return EFI_SUCCESS;
}


/**
 * カーネルファイル内のすべてのLOADセグメント(p_type が PT_LOADであるセグメント)を走査し、
 * アドレス範囲を更新します。
 */
VOID CalcLoadAddressRange(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last) {
  Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
  *first = MAX_UINT64;
  *last = 0;

  for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
    if (phdr[i].p_type != PT_LOAD) {
      continue;
    }
    *first = MIN(*first, phdr[i].p_vaddr);
    *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
  }
}

/**
 * p_type == PT_LOAD であるセグメントに対して2つの処理を行う
 * 1. segm_in_fileが指す一時領域から p_vaddr が指す最終目的地へデータをコピーする
 * 2. セグメントのメモリ上のサイズがファイル上のサイズより大きい場合(remain_bytes > 0)、残りを0で埋める(SetMem())
 */
VOID CopyLoadSegments(Elf64_Ehdr* ehdr) {
  Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
  for (Elf64_Half i = 0; i < ehdr->e_phnum; i++) {
    if (phdr[i].p_type != PT_LOAD) {
      continue;
    }

    // 一時領域から最終目的地へデータをコピー
    UINT64 segm_in_file = (UINT64)ehdr + phdr[i].p_offset;
    // CopyMem: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Library/BaseMemoryLib.h#L33
    CopyMem(
      (VOID*)phdr[i].p_vaddr,  // IN VOID   *Destination
      (VOID*)segm_in_file,     // IN VOID   *Source
      phdr[i].p_filesz         // IN UINTN  Length
    );

    // 残りのメモリを0埋め
    UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
    // SetMem: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Library/BaseMemoryLib.h#L55
    SetMem(
      (VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz),  // IN VOID   *Buffer
      remain_bytes,                                 // IN UINTN  Size
      0                                             // IN UINT8  Value
    );
  }
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop) {
  UINTN num_gop_handles = 0;
  EFI_HANDLE* gop_handles = NULL;
  // EFI_LOCATE_HANDLE_BUFFER: https://github.com/tianocore/edk2/blob/edk2-stable202208/MdePkg/Include/Uefi/UefiSpec.h#L1570
  // リクエストされたプロトコルをサポートするハンドルの配列を取得する
  gBS->LocateHandleBuffer(
    ByProtocol,                      // IN  EFI_LOCATE_SEARCH_TYPE  SearchType            返却されるハンドルの指定
    &gEfiGraphicsOutputProtocolGuid, // IN  EFI_GUID                *Protocol   OPTIONAL  検索するプロトコルを指定(SearchType=ByProtocolの場合のみ有効)
    NULL,                            // IN  VOID                    *SearchKey  OPTIONAL  SearchType に応じた検索キーを指定
    &num_gop_handles,                // OUT UINTN                   *NoHandles            取得したハンドルの数
    &gop_handles                     // OUT EFI_HANDLE              **Buffer              バッファから返却されたハンドルの配列
  );
  // EFI_OPEN_PROTOCOL: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L1330
  //   ハンドルが指定されたプロトコルをサポートしているかを問い合わせ、 サポートしている場合、呼び出し元のエージェントに代わってプロトコルをオープンする
  gBS->OpenProtocol(
    gop_handles[0],
    &gEfiGraphicsOutputProtocolGuid,
    (VOID**)gop,
    image_handle,
    NULL,
    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
  );
  return EFI_SUCCESS;
}

/**
 * EFI_GRAPHICS_PIXEL_FORMATを文字列に変換する
 */
const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt) {
  // EFI_GRAPHICS_PIXEL_FORMAT: https://github.com/tianocore/edk2/blob/edk2-stable202208/MdePkg/Include/Protocol/GraphicsOutput.h#L28
  switch (fmt) {
    case PixelRedGreenBlueReserved8BitPerColor:
      return L"PixelRedGreenBlueReserved8BitPerColor";
    case PixelBlueGreenRedReserved8BitPerColor:
      return L"PixelBlueGreenRedReserved8BitPerColor";
    case PixelBitMask:
      return L"PixelBitMask";
    case PixelBltOnly:
      return L"PixelBltOnly";
    case PixelFormatMax:
      return L"PixelFormatMax";
    default:
      return L"InvalidPixelFormat";
  }
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
  EFI_STATUS status;
  Print(L"Hello, MikanOS!\n");

  /**
   * メモリマップを取得する
   */
  CHAR8 memmap_buf[4096 * 4]; // 4page分
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};
  GetMemoryMap(&memmap);

  /**
   * メモリマップを保存するファイルを開く
   */
  // EFI_FILE_PROTOCOL: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L528
  //   ファイルシステム上のファイルやディレクトリに対する入出力操作を行うためのインターフェース
  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL* memmap_file;
  // EFI_FILE_OPEN: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L113
  root_dir->Open(
    root_dir,     // IN EFI_FILE_PROTOCOL *This
    &memmap_file, // OUT EFI_FILE_PROTOCOL **NewHandle
    L"\\memmap",  // IN CHAR16 *FileName
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,  // IN UINT64 OpenMode
    0             // IN UINT64 Attributes
  );

  /**
   * メモリマップをファイルに書き出す
   */
  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  /**
   * GOPを取得して画面描画する
   */
  // EFI_GRAPHICS_OUTPUT_PROTOCOL: https://github.com/tianocore/edk2/blob/edk2-stable202208/MdePkg/Include/Protocol/GraphicsOutput.h#L258
  EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
  OpenGOP(image_handle, &gop);
  // EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE: https://github.com/tianocore/edk2/blob/edk2-stable202208/MdePkg/Include/Protocol/GraphicsOutput.h#L224
  Print(L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
    gop->Mode->Info->HorizontalResolution,  // 水平方向のピクセル数
    gop->Mode->Info->VerticalResolution,    // 垂直方向のピクセル数
    GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),  // フレームバッファで1ピクセルを表すデータ形式
    gop->Mode->Info->PixelsPerScanLine  // ビデオメモリラインあたりのピクセル数
  );
  Print(L"Frame Buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
    gop->Mode->FrameBufferBase,  // フレームバッファの先頭アドレス
    gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,  // フレームバッファの末尾アドレス
    gop->Mode->FrameBufferSize  // フレームバッファの全体サイズ
  );

  UINT8* frame_buffer = (UINT8*)gop->Mode->FrameBufferBase;
  for (UINTN i = 0; i < gop->Mode->FrameBufferSize; i++) {
    frame_buffer[i] = 255;
  }


  /**
   * カーネルファイルを読み取り専用で開く
   */
  EFI_FILE_PROTOCOL* kernel_file;
  root_dir->Open(
    root_dir,
    &kernel_file,
    L"\\kernel.elf",
    EFI_FILE_MODE_READ,
    0
  );

  /**
   * カーネルファイルのファイルサイズを取得
   */
  // EFI_FILE_INFO型を十分格納できる大きさのメモリを確保
  // FileNameは可変長なのでファイル名が収まるくらいのメモリを追加で確保する必要がある
  //
  // EFI_FILE_INFO: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Guid/FileInfo.h#L19
  //   UINT64    Size;
  //   UINT64    FileSize;
  //   UINT64    PhysicalSize;
  //   EFI_TIME  CreateTime;
  //   EFI_TIME  LastAccessTime;
  //   EFI_TIME  ModificationTime;
  //   UINT64    Attribute;
  //   CHAR16    FileName[1];
  UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
  // UINT8: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/X64/ProcessorBind.h#L167
  // 1byte符号なし整数
  UINT8 file_info_buffer[file_info_size];

  // kernel_file を file_info_buffer に展開
  // EFI_FILE_GET_INFO: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L287
  kernel_file->GetInfo(
    kernel_file,        // IN EFI_FILE_PROTOCOL  *This
    &gEfiFileInfoGuid,  // IN EFI_GUID           *InformationType
    &file_info_size,    // IN OUT UINTN          *BufferSize
    file_info_buffer    // OUT VOID              *Buffer
  );

  EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
  // カーネルファイルのファイルサイズを取得
  UINTN kernel_file_size = file_info->FileSize;

  /**
   * カーネルファイルを一時領域に読み込む
   */
  VOID* kernel_buffer;
  // EFI_ALLOCATE_POOL: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L268
  gBS->AllocatePool(
    EfiLoaderData,     // IN  EFI_MEMORY_TYPE  PoolType,
    kernel_file_size,  // IN  UINTN            Size,
    &kernel_buffer     // OUT VOID             **Buffer
  );
  kernel_file->Read(
    kernel_file,             // IN EFI_FILE_PROTOCOL  *This
    &kernel_file_size,       // IN OUT UINTN          *BufferSize
    (VOID*)kernel_buffer     // OUT VOID              *Buffer
  );

  /**
   * コピー先のメモリを確保
   */
  Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
  UINT64 kernel_first_addr, kernel_last_addr;
  CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);
  
  UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
  // EFI_ALLOCATE_PAGES: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L186
  status = gBS->AllocatePages(
    AllocateAddress,   // IN     EFI_ALLOCATE_TYPE    Type : https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L29
                       //   AllocateAnyPages: どこでもいいからアイている場所に確保
                       //   AllocateMaxAddress: 指定したアドレス以下で空いている場所に確保
                       //   AllocateAddress: 指定したアドレスに確保
    EfiLoaderData,     // IN     EFI_MEMORY_TYPE      MemoryType : https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiMultiPhase.h#L38
                       //   UEFI アプリケーションやドライバがメモリを割り当てる際、どのような目的で使用するメモリかを指定する
                       //   EfiLoaderCode: ロードされたアプリケーションのコードセクション
                       //   EfiLoaderData: ロードされたアプリケーションのデータセクション
    num_pages,         // IN     UINTN                Pages
    &kernel_first_addr // IN OUT EFI_PHYSICAL_ADDRESS *Memory
  );

  /**
   * LOADセグメントを確保したメモリ領域にコピー
   */
  CopyLoadSegments(kernel_ehdr);
  Print(L"Kernel: 0x%0lx - 0x%lx\n", kernel_first_addr, kernel_last_addr);

  // EFI_FREE_POOL: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h?utm_source=chatgpt.com#L285
  status = gBS->FreePool(kernel_buffer);

  /**
   * カーネルの起動
   */
  // UINT64: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/X64/ProcessorBind.h#L180
  // メモリ上でエントリーポイントがおいてあるアドレス
  // ELF形式の仕様では64bit用のELFのエントリポイントアドレスはオフセット24byteの位置から8バイト整数として書かれる事になっている
  // ELFの情報は readelf -h build/kernel/kernel.elf で確認できる
  UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 24);

  // エントリポイントをC言語の関数として呼び出すために、関数ポインタにキャスト
  typedef void EntryPointType(void);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  Print(L"entry_point: 0x%p\n", entry_point);

  /**
   * カーネル起動前にUEFI BIOSのブートサービスを停止
   */
  // EFI_EXIT_BOOT_SERVICES: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L983
  //   ブートサービスを停止させる。この関数が成功した場合、以降にブートサービスの機能を使うことはできない。
  status = gBS->ExitBootServices(
    image_handle,  // IN  EFI_HANDLE ImageHandle
    memmap.map_key // IN  UINTN      MapKey       最新のメモリマップのマップキーを要求。マップキーが最新のメモリマップに紐づく値でない場合は失敗する
  );
  if (EFI_ERROR(status)) { // 停止に失敗した場合はリトライ
    // 最新のメモリマップを取得
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
      Print(L"failed to get memory map: %r\n", status);
      while (1);
    }
    // 停止をリトライ
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
      Print(L"Could not exit boot service: %r\n", status);
      while (1);
    }
  }

  // 関数ポインタを実行
  entry_point();

  Print(L"All done\n");

  while(1);
  return EFI_SUCCESS;

}