#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>

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
  // gBS->GetMemoryMap()
  //   呼び出し時点のメモリマップを取得し、MemoryMap（第二引数)で指定されたメモリ領域に書き込みます。
  //   正常にメモリマップが取得できると EFI_SUCCESS を返却。メモリ領域が小さくてメモリマップを書き込めない場合は EFI_BUFFER_TOO_SMALL を返却。
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L239
  return gBS->GetMemoryMap(
    // メモリマップ書き込み用のメモリ領域のサイズ(バイト)
    &map->map_size,
    // メモリマップ書き込み用メモリ領域の先頭ポインタ。メモリディスクリプタ(構造体)の配列
    // EFI_MEMORY_DESCRIPTOR: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L128
    //   UINT32                Type           メモリ領域の種別
    //   EFI_PHYSICAL_ADDRESS  PhysicalStart  メモリ領域先頭の物理メモリアドレス
    //   EFI_VIRTUAL_ADDRESS   VirtualStart   メモリ領域先頭の仮想メモリアドレス
    //   UINT64                NumberOfPages  メモリ領域の大きさ (4KiBページ単位)
    //   UINT64                Attribute      メモリ領域が使える用途を示すビット集合
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    // メモリマップを識別するための値を書き込む変数。メモリマップはプログラムの処理やUEFI自体の処理によって変化します。
    // 2つの時点で取得したMapKeyの値が同じなら、その時間内ではメモリマップに変化がないということを意味します。
    &map->map_key,
    // メモリマップの個々の行を表すメモリディスクリプタのバイト数を表します。
    &map->descriptor_size,
    // メモリディスクリプタの構造体のバージョン番号
    &map->descriptor_version
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
  UINTN len;

  CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Library/BaseLib.h#L1673
  len = AsciiStrLen(header);
  // EFI_FILE_PROTOCOL.Write: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L539
  // EFI_FILE_WRITE: https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L220
  //   IN EFI_FILE_PROTOCOL  *This,
  //   IN OUT UINTN          *BufferSize,
  //   IN VOID               *Buffer
  file->Write(file, &len, header);

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
  //   IN  EFI_HANDLE                Handle,
  //   IN  EFI_GUID                  *Protocol,
  //   OUT VOID                      **Interface  OPTIONAL,
  //   IN  EFI_HANDLE                AgentHandle,
  //   IN  EFI_HANDLE                ControllerHandle,
  //   IN  UINT32                    Attributes
  gBS->OpenProtocol(
    image_handle,
    &gEfiLoadedImageProtocolGuid,
    (VOID**)&loaded_image,
    image_handle,
    NULL,
    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
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
  //   IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  //   OUT EFI_FILE_PROTOCOL                **Root
  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}


EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE* system_table) {
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
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L113
  root_dir->Open(
    root_dir,     // IN EFI_FILE_PROTOCOL *This
    &memmap_file, // OUT EFI_FILE_PROTOCOL **NewHandle
    L"\\memmap",  // IN CHAR16 *FileName
    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,  // IN UINT64 OpenMode
    0             // IN UINT64 Attributes
  );

  /**
   * メモリマップを保存する
   */
  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  Print(L"All done\n");

  while(1);
  return EFI_SUCCESS;
}