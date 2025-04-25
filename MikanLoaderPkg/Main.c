#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>

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

EFI_STATUS GetMemoryMap(struct MemoryMap *map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L239
  return gBS->GetMemoryMap(
    &map->map_size,
    // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L128
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    &map->map_key,
    &map->descriptor_size,
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

EFI_STATUS SaveMemoryMap(
  struct MemoryMap* map,
  EFI_FILE_PROTOCOL* file
) {
  CHAR8 buf[256];
  UINTN len;

  CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Library/BaseLib.h#L1673
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiBaseType.h#L50
  EFI_PHYSICAL_ADDRESS iter;
  int i;
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++
  ) {
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
    len = AsciiSPrint(
      buf,
      sizeof(buf),
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
 * 
 */
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root) {
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L73
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Uefi/UefiSpec.h#L1330
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

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}


EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE* system_table) {
  Print(L"Hello, MikanOS!\n");
  CHAR8 memmap_buf[4096 * 4]; // 4page分
  struct MemoryMap memmap = {sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0};

  GetMemoryMap(&memmap);

  // https://github.com/tianocore/edk2/blob/edk2-stable202302/MdePkg/Include/Protocol/SimpleFileSystem.h#L528
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

  SaveMemoryMap(&memmap, memmap_file);
  memmap_file->Close(memmap_file);

  Print(L"All done\n");

  while(1);
  return EFI_SUCCESS;
}