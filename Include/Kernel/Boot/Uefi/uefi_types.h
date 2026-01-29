#pragma once

#include <LibFK/Types/types.h>

namespace uefi {

// UEFI Specification types
typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

// UEFI Status codes
constexpr EFI_STATUS EFI_SUCCESS = 0;
constexpr EFI_STATUS EFI_ERROR_MASK = 0x8000000000000000ULL;
constexpr EFI_STATUS EFI_LOAD_ERROR = (EFI_ERROR_MASK | 1);
constexpr EFI_STATUS EFI_INVALID_PARAMETER = (EFI_ERROR_MASK | 2);
constexpr EFI_STATUS EFI_UNSUPPORTED = (EFI_ERROR_MASK | 3);
constexpr EFI_STATUS EFI_NO_MEDIA = (EFI_ERROR_MASK | 13);
constexpr EFI_STATUS EFI_NOT_FOUND = (EFI_ERROR_MASK | 14);

// UEFI Memory Types
enum class EFI_MEMORY_TYPE : uint32_t {
  EfiReservedMemoryType = 0,
  EfiLoaderCode = 1,
  EfiLoaderData = 2,
  EfiBootServicesCode = 3,
  EfiBootServicesData = 4,
  EfiRuntimeServicesCode = 5,
  EfiRuntimeServicesData = 6,
  EfiConventionalMemory = 7,
  EfiUnusableMemory = 8,
  EfiACPIReclaimMemory = 9,
  EfiACPIMemoryNVS = 10,
  EfiMemoryMappedIO = 11,
  EfiMemoryMappedIOPortSpace = 12,
  EfiPalCode = 13,
  EfiMaxMemoryType = 14
};

// UEFI Table Header
struct EFI_TABLE_HEADER {
  uint64_t Signature;
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t CRC32;
  uint32_t Reserved;
};

// EFI Simple Text Input Protocol
struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
  void *Reset;
  void *ReadKeyStroke;
  EFI_HANDLE WaitForKey;
};

// EFI Simple Text Output Protocol
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  void *Reset;
  void *OutputString;
  void *TestString;
  void *QueryMode;
  void *SetMode;
  void *SetAttribute;
  void *ClearScreen;
  void *SetCursorPosition;
  void *EnableCursor;
  void *Mode;
};

// Forward declarations and typedefs for types used in function pointers
struct EFI_MEMORY_DESCRIPTOR;
struct EFI_GUID;
struct EFI_BLOCK_IO_MEDIA;
typedef uint64_t EFI_LBA;

// Function pointer type for SetVirtualAddressMap
typedef EFI_STATUS (*EFI_SET_VIRTUAL_ADDRESS_MAP)(
    size_t MemoryMapSize,
    size_t DescriptorSize,
    uint32_t DescriptorVersion,
    EFI_MEMORY_DESCRIPTOR *VirtualMap
);

// EFI Runtime Services
struct EFI_RUNTIME_SERVICES {
  EFI_TABLE_HEADER Hdr;
  EFI_SET_VIRTUAL_ADDRESS_MAP *SetVirtualAddressMap;
  // ... other fields
};

// EFI Locate Search Type (must be defined before function pointer types)
enum EFI_LOCATE_SEARCH_TYPE {
  AllHandles,
  ByRegisterNotify,
  ByProtocol
};

// Function pointer types for EFI Boot Services (must be defined before EFI_BOOT_SERVICES)
typedef EFI_STATUS (*EFI_GET_MEMORY_MAP)(
    size_t *MemoryMapSize,
    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    size_t *MapKey,
    size_t *DescriptorSize,
    uint32_t *DescriptorVersion
);

typedef EFI_STATUS (*EFI_ALLOCATE_POOL)(
    uint32_t PoolType,
    size_t Size,
    void **Buffer
);

typedef EFI_STATUS (*EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE Handle,
    const EFI_GUID *Protocol,
    void **Interface
);

typedef EFI_STATUS (*EFI_LOCATE_PROTOCOL)(
    const EFI_GUID *Protocol,
    void *Registration,
    void **Interface
);

typedef EFI_STATUS (*EFI_LOCATE_HANDLE_BUFFER)(
    EFI_LOCATE_SEARCH_TYPE SearchType,
    const EFI_GUID *Protocol,
    void *SearchKey,
    size_t *NoHandles,
    EFI_HANDLE **Buffer
);

typedef EFI_STATUS (*EFI_EXIT_BOOT_SERVICES)(
    EFI_HANDLE ImageHandle,
    size_t MapKey
);

// EFI Boot Services
struct EFI_BOOT_SERVICES {
  EFI_TABLE_HEADER Hdr;
  
  // Task Priority Services
  void *RaiseTPL;
  void *RestoreTPL;
  
  // Memory Services
  void *AllocatePages;
  void *FreePages;
  EFI_GET_MEMORY_MAP *GetMemoryMap;
  EFI_ALLOCATE_POOL *AllocatePool;
  void *FreePool;
  
  // Event & Timer Services
  void *CreateEvent;
  void *SetTimer;
  void *WaitForEvent;
  void *SignalEvent;
  void *CloseEvent;
  void *CheckEvent;
  
  // Protocol Handler Services
  void *InstallProtocolInterface;
  void *ReinstallProtocolInterface;
  void *UninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL *HandleProtocol;
  void *Reserved;
  void *RegisterProtocolNotify;
  void *LocateHandle;
  void *LocateDevicePath;
  void *InstallConfigurationTable;
  
  // Image Services
  void *LoadImage;
  void *StartImage;
  void *Exit;
  void *UnloadImage;
  EFI_EXIT_BOOT_SERVICES *ExitBootServices;
  
  // Miscellaneous Services
  void *GetNextMonotonicCount;
  void *Stall;
  void *SetWatchdogTimer;
  
  // Driver Support Services
  void *ConnectController;
  void *DisconnectController;
  
  // Open and Close Protocol Services
  void *OpenProtocol;
  void *CloseProtocol;
  void *OpenProtocolInformation;
  
  // Library Services
  void *ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER *LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL *LocateProtocol;
  void *InstallMultipleProtocolInterfaces;
  void *UninstallMultipleProtocolInterfaces;
  
  // 32-bit CRC Services
  void *CalculateCrc32;
  
  // Miscellaneous Services
  void *CopyMem;
  void *SetMem;
  void *CreateEventEx;
};

// EFI System Table
struct EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER Hdr;
  char16_t *FirmwareVendor;
  uint32_t FirmwareRevision;
  EFI_HANDLE ConsoleInHandle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  EFI_HANDLE ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
  EFI_HANDLE StandardErrorHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
  EFI_RUNTIME_SERVICES *RuntimeServices;
  EFI_BOOT_SERVICES *BootServices;
  size_t NumberOfTableEntries;
  void **ConfigurationTable;
};

// EFI Memory Descriptor
struct EFI_MEMORY_DESCRIPTOR {
  uint32_t Type;
  uint32_t Padding;
  EFI_PHYSICAL_ADDRESS PhysicalStart;
  EFI_VIRTUAL_ADDRESS VirtualStart;
  uint64_t NumberOfPages;
  uint64_t Attribute;
};

// Graphics Output Protocol
struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
  void *QueryMode;
  void *SetMode;
  void *Blt;
  
  struct {
    uint32_t MaxMode;
    uint32_t Mode;
    struct {
      uint32_t Version;
      uint32_t HorizontalResolution;
      uint32_t VerticalResolution;
      uint32_t PixelFormat;
      struct {
        uint32_t RedMask;
        uint32_t GreenMask;
        uint32_t BlueMask;
        uint32_t ReservedMask;
      } PixelInformation;
      uint32_t PixelsPerScanLine;
    } *ModeInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    size_t FrameBufferSize;
  } *Mode;
};

// Block I/O Protocol
struct EFI_BLOCK_IO_PROTOCOL {
  uint64_t Revision;
  EFI_BLOCK_IO_MEDIA *Media;
  
  void *Reset;
  void *ReadBlocks;
  void *WriteBlocks;
  void *FlushBlocks;
};

struct EFI_BLOCK_IO_MEDIA {
  uint32_t MediaId;
  bool RemovableMedia;
  bool MediaPresent;
  bool LogicalPartition;
  bool ReadOnly;
  bool WriteCaching;
  uint32_t BlockSize;
  uint32_t IoAlign;
  EFI_LBA LastBlock;
};

// EFI Loaded Image Protocol
struct EFI_LOADED_IMAGE_PROTOCOL {
  uint32_t Revision;
  EFI_HANDLE ParentHandle;
  EFI_SYSTEM_TABLE *SystemTable;
  EFI_HANDLE DeviceHandle;
  void *FilePath;
  void *Reserved;
  uint32_t LoadOptionsSize;
  void *LoadOptions;
  void *ImageBase;
  uint64_t ImageSize;
  uint32_t ImageCodeType;
  uint32_t ImageDataType;
  void *Unload;
};

// EFI Device Path Protocol
struct EFI_DEVICE_PATH_PROTOCOL {
  uint8_t Type;
  uint8_t SubType;
  uint16_t Length;
};

// EFI File Handle (opaque pointer)
struct EFI_FILE_PROTOCOL;
typedef EFI_FILE_PROTOCOL *EFI_FILE_HANDLE;

// EFI Simple File System Protocol
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  uint64_t Revision;
  
  typedef EFI_STATUS (*EFI_OPEN_VOLUME)(
      EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
      EFI_FILE_HANDLE *Root
  );
  
  EFI_OPEN_VOLUME *OpenVolume;
};

// EFI File Protocol
struct EFI_FILE_PROTOCOL {
  uint64_t Revision;
  
  typedef EFI_STATUS (*EFI_FILE_OPEN)(
      EFI_FILE_PROTOCOL *This,
      EFI_FILE_HANDLE *NewHandle,
      const uint16_t *FileName,
      uint64_t OpenMode,
      uint64_t Attributes
  );
  
  typedef EFI_STATUS (*EFI_FILE_CLOSE)(
      EFI_FILE_PROTOCOL *This
  );
  
  typedef EFI_STATUS (*EFI_FILE_READ)(
      EFI_FILE_PROTOCOL *This,
      size_t *BufferSize,
      void *Buffer
  );
  
  typedef EFI_STATUS (*EFI_FILE_SET_POSITION)(
      EFI_FILE_PROTOCOL *This,
      uint64_t Position
  );
  
  EFI_FILE_OPEN *Open;
  EFI_FILE_CLOSE *Close;
  EFI_FILE_READ *Read;
  EFI_FILE_SET_POSITION *SetPosition;
  // ... other file operations omitted for brevity
};

// Simple File System Protocol GUID
constexpr uint64_t EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_DATA1 = 0x0964e5b22;
constexpr uint16_t EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_DATA2 = 0x6459;
constexpr uint16_t EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_DATA3 = 0x11d2;
constexpr uint8_t EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_DATA4[8] = {
  0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b
};

// ACPI 2.0 Table GUID
constexpr uint64_t ACPI_20_TABLE_GUID_PART1 = 0x8868e871;
constexpr uint64_t ACPI_20_TABLE_GUID_PART2 = 0xe4b0a1c2;
constexpr uint64_t ACPI_20_TABLE_GUID_PART3 = 0x49b9e6e5;
constexpr uint64_t ACPI_20_TABLE_GUID_PART4 = 0x27237a51;

// ACPI Table GUID structure
struct EFI_GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
};

// Simple File System Protocol GUID
extern const EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

// Configuration Table Entry
struct EFI_CONFIGURATION_TABLE {
  EFI_GUID VendorGuid;
  void *VendorTable;
};

} // namespace uefi
