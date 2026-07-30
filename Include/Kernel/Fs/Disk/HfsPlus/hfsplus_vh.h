#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

// Byte-swap helpers for big-endian HFS+ on-disk structures
static inline uint16_t hfs_be16(uint16_t x) {
    return (uint16_t)((x >> 8) | (x << 8));
}
static inline uint32_t hfs_be32(uint32_t x) {
    return ((x >> 24) & 0xFF) | ((x >> 8) & 0xFF00) |
           ((x << 8) & 0xFF0000) | ((x << 24) & 0xFF000000);
}
static inline uint64_t hfs_be64(uint64_t x) {
    return ((uint64_t)hfs_be32((uint32_t)(x >> 32))) |
           ((uint64_t)hfs_be32((uint32_t)(x & 0xFFFFFFFF)) << 32);
}

// Volume header signatures
static constexpr uint16_t kHFSPlusSig  = 0x482B; // 'H+'
static constexpr uint16_t kHFSXSig     = 0x4858; // 'HX'
static constexpr uint16_t kHFSPlusVers = 4;
static constexpr uint16_t kHFSXVers    = 5;

// Well-known CNIDs
static constexpr uint32_t kHFSRootParentID     = 1;
static constexpr uint32_t kHFSRootFolderID     = 2;
static constexpr uint32_t kHFSExtentsFileID    = 3;
static constexpr uint32_t kHFSCatalogFileID    = 4;
static constexpr uint32_t kHFSBadBlockFileID   = 5;
static constexpr uint32_t kHFSAllocationFileID = 6;
static constexpr uint32_t kHFSStartupFileID    = 7;
static constexpr uint32_t kHFSAttributesFileID = 8;
static constexpr uint32_t kHFSFirstUserCNID    = 16;

// Volume attribute flags
static constexpr uint32_t kHFSVolumeUnmountedBit = 8;

// Extent descriptor: (startBlock, blockCount)
struct HFSPlusExtentDescriptor {
    uint32_t startBlock;
    uint32_t blockCount;
} __attribute__((packed));

static constexpr int kHFSPlusExtentDensity = 8;

// Fork data: 8 inline extents
struct HFSPlusForkData {
    uint64_t logicalSize;
    uint32_t clumpSize;
    uint32_t totalBlocks;
    HFSPlusExtentDescriptor extents[kHFSPlusExtentDensity];
} __attribute__((packed));

// On-disk Volume Header at byte offset 1024 from partition start
struct HFSPlusVolumeHeader {
    uint16_t signature;       // kHFSPlusSig or kHFSXSig
    uint16_t version;
    uint32_t attributes;
    uint32_t lastMountedVersion;
    uint32_t journalInfoBlock;
    uint32_t createDate;
    uint32_t modifyDate;
    uint32_t backupDate;
    uint32_t checkedDate;
    uint32_t fileCount;
    uint32_t folderCount;
    uint32_t blockSize;       // allocation block size (bytes)
    uint32_t totalBlocks;
    uint32_t freeBlocks;
    uint32_t nextAllocation;
    uint32_t rsrcClumpSize;
    uint32_t dataClumpSize;
    uint32_t nextCatalogID;
    uint32_t writeCount;
    uint64_t encodingsBitmap;
    uint32_t finderInfo[8];
    HFSPlusForkData allocationFile;
    HFSPlusForkData extentsFile;
    HFSPlusForkData catalogFile;
    HFSPlusForkData attributesFile;
    HFSPlusForkData startupFile;
} __attribute__((packed));

// Catalog record types
static constexpr int16_t kHFSPlusFolderRecord       = 0x0001;
static constexpr int16_t kHFSPlusFileRecord         = 0x0002;
static constexpr int16_t kHFSPlusFolderThreadRecord = 0x0003;
static constexpr int16_t kHFSPlusFileThreadRecord   = 0x0004;

// HFS+ Unicode string
struct HFSUniStr255 {
    uint16_t length;
    uint16_t unicode[255];
} __attribute__((packed));

// Catalog key
struct HFSPlusCatalogKey {
    uint16_t    keyLength;
    uint32_t    parentID;
    HFSUniStr255 nodeName;
} __attribute__((packed));

// FinderInfo for files
struct HFSPlusPoint  { int16_t v, h; } __attribute__((packed));
struct HFSPlusRect   { int16_t top, left, bottom, right; } __attribute__((packed));

struct FInfo {
    uint32_t fdType;
    uint32_t fdCreator;
    uint16_t fdFlags;
    HFSPlusPoint fdLocation;
    int16_t  fdFldr;
} __attribute__((packed));

struct FXInfo {
    int16_t  fdIconID;
    int16_t  fdUnused[3];
    int8_t   fdScript;
    int8_t   fdXFlags;
    int16_t  fdComment;
    int32_t  fdPutAway;
} __attribute__((packed));

struct DInfo {
    HFSPlusRect frRect;
    uint16_t frFlags;
    HFSPlusPoint frLocation;
    int16_t  frView;
} __attribute__((packed));

struct DXInfo {
    HFSPlusPoint frScroll;
    int32_t  frOpenChain;
    int16_t  frUnused;
    int16_t  frComment;
    int32_t  frPutAway;
} __attribute__((packed));

// Catalog folder record
struct HFSPlusCatalogFolder {
    int16_t  recordType;
    uint16_t flags;
    uint32_t valence;
    uint32_t folderID;
    uint32_t createDate;
    uint32_t contentModDate;
    uint32_t attributeModDate;
    uint32_t accessDate;
    uint32_t backupDate;
    uint32_t ownerID;
    uint32_t groupID;
    uint8_t  adminFlags;
    uint8_t  ownerFlags;
    uint16_t fileMode;
    uint32_t special;
    DInfo    userInfo;
    DXInfo   finderInfo;
    uint32_t textEncoding;
    uint32_t folderCount;
} __attribute__((packed));

// Catalog file record
struct HFSPlusCatalogFile {
    int16_t  recordType;
    uint16_t flags;
    uint32_t reserved1;
    uint32_t fileID;
    uint32_t createDate;
    uint32_t contentModDate;
    uint32_t attributeModDate;
    uint32_t accessDate;
    uint32_t backupDate;
    uint32_t ownerID;
    uint32_t groupID;
    uint8_t  adminFlags;
    uint8_t  ownerFlags;
    uint16_t fileMode;
    uint32_t special;
    FInfo    userInfo;
    FXInfo   finderInfo;
    uint32_t textEncoding;
    uint32_t reserved2;
    HFSPlusForkData dataFork;
    HFSPlusForkData resourceFork;
} __attribute__((packed));

// Thread record
struct HFSPlusCatalogThread {
    int16_t     recordType;
    int16_t     reserved;
    uint32_t    parentID;
    HFSUniStr255 nodeName;
} __attribute__((packed));

// B-tree node descriptor
struct BTNodeDescriptor {
    uint32_t fLink;      // next node number
    uint32_t bLink;      // previous node number
    int8_t   kind;       // node type
    uint8_t  height;     // level in tree (leaf = 1)
    uint16_t numRecords; // number of records
    uint16_t reserved;
} __attribute__((packed));

// B-tree node types
static constexpr int8_t kBTLeafNode   = -1;
static constexpr int8_t kBTIndexNode  = 0;
static constexpr int8_t kBTHeaderNode = 1;
static constexpr int8_t kBTMapNode    = 2;

// B-tree header record (first record in the header node)
struct BTHeaderRec {
    uint16_t treeDepth;
    uint32_t rootNode;
    uint32_t leafRecords;
    uint32_t firstLeafNode;
    uint32_t lastLeafNode;
    uint16_t nodeSize;
    uint16_t maxKeyLength;
    uint32_t totalNodes;
    uint32_t freeNodes;
    uint16_t reserved1;
    uint32_t clumpSize;
    uint8_t  btreeType;
    uint8_t  keyCompareType;
    uint32_t attributes;
    uint32_t reserved3[16];
} __attribute__((packed));

// B-tree attributes
static constexpr uint32_t kBTBigKeysMask     = 0x00000002;
static constexpr uint32_t kBTVariableIndexKeysMask = 0x00000004;

// Extents key
struct HFSPlusExtentKey {
    uint16_t keyLength;
    uint8_t  forkType;
    uint8_t  pad;
    uint32_t fileID;
    uint32_t startBlock;
} __attribute__((packed));

// keyCompareType for HFSX
static constexpr uint8_t kHFSCaseFolding   = 0xCF;
static constexpr uint8_t kHFSBinaryCompare = 0xBC;

} // namespace fkernel
