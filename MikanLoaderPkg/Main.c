#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include  <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include  <Guid/FileInfo.h>

// メモリマップの構造体
// UINTN CPUアーキテクチャ依存の符号なし整数型
//       32bitでは unsigned int (4byte)
//       64bitでは unsigned long (8byte)
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
  // gBS
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
  gBS->OpenProtocol(
    image_handle,                        // IN  EFI_HANDLE  Handle,
    &gEfiLoadedImageProtocolGuid,        // IN  EFI_GUID    *Protocol,
    (VOID**)&loaded_image,               // OUT VOID        **Interface  OPTIONAL,
    image_handle,                        // IN  EFI_HANDLE  AgentHandle,
    NULL,                                // IN  EFI_HANDLE  ControllerHandle,
    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL // IN  UINT32      Attributes
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
  fs->OpenVolume(
    fs,  // IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
    root // OUT EFI_FILE_PROTOCOL                **Root
  );

  return EFI_SUCCESS;
}


EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
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
   * カーネルファイルをメモリに展開
   */
  // カーネルファイルは ld.lld の --image-base オプションで 0x100000 に配置して動作させる前提で作ってある
  EFI_PHYSICAL_ADDRESS kernel_base_addr = 0x100000;
  // メモリの確保
  // EFI_ALLOCATE_PAGES: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L211
  gBS->AllocatePages(
    // EFI_ALLOCATE_TYPE: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L29
    //   AllocateAnyPages: どこでもいいからアイている場所に確保
    //   AllocateMaxAddress: 指定したアドレス以下で空いている場所に確保
    //   AllocateAddress: 指定したアドレスに確保
    AllocateAddress,                     // IN     EFI_ALLOCATE_TYPE     Type,       // メモリ確保の方法
    // EFI_MEMORY_TYPE: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiMultiPhase.h#L38
    EfiLoaderData,                       // IN     EFI_MEMORY_TYPE       MemoryType, // 確保するメモリ領域の種別
    // 0xfffは除算で切り捨てされてしまう問題の対応
    (kernel_file_size + 0xfff) / 0x1000, // IN     UINTN                 Pages,      // 確保するメモリのサイズ(ページ単位なので4KiB(0x1000バイト)で除算)
    &kernel_base_addr                    // IN OUT EFI_PHYSICAL_ADDRESS  *Memory     // 確保したメモリ領域のアドレスを書き込む変数
  );
  // EFI_FILE_READ: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L192
  kernel_file->Read(
    kernel_file,             // IN EFI_FILE_PROTOCOL  *This
    &kernel_file_size,       // IN OUT UINTN          *BufferSize
    (VOID*)kernel_base_addr  // OUT VOID              *Buffer
  );
  Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_addr, kernel_file_size);

  /**
   * カーネルの起動
   */
  // UINT64: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/X64/ProcessorBind.h#L180
  // メモリ上でエントリーポイントがおいてあるアドレス
  // ELF形式の仕様では64bit用のELFのエントリポイントアドレスはオフセット24byteの位置から8バイト整数として書かれる事になっている
  // ELFの情報は readelf -h build/kernel/kernel.elf で確認できる
  UINT64 entry_addr = *(UINT64*)(kernel_base_addr + 24);

  // エントリポイントをC言語の関数として呼び出すために、関数ポインタにキャスト
  typedef void EntryPointType(void);
  EntryPointType* entry_point = (EntryPointType*)entry_addr;
  Print(L"entry_point: 0x%p\n", entry_point);

  /**
   * カーネル起動前にUEFI BIOSのブートサービスを停止
   */
  EFI_STATUS status;
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