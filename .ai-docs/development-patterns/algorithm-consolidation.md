# Algorithm Consolidation Policy

## Overview

This document establishes the policy for consolidating known algorithms used across multiple FKernel domains into `LibFK/Algorithms/` for maximum reusability and maintainability.

## Policy Statement

**All known algorithms used across multiple kernel domains MUST be consolidated in `LibFK/Algorithms/` rather than being duplicated in domain-specific implementations.**

## Algorithm Categories

### 1. Archive Algorithms
- **TAR**: Archive extraction and creation
- **ZIP**: ZIP archive support
- **GZIP**: Compression/decompression
- **Usage**: Filesystem, Loader, Userland tools

### 2. Compression Algorithms  
- **LZ4**: Fast compression
- **ZLIB**: Standard compression
- **DEFLATE**: Basic compression
- **Usage**: Storage devices, Network protocols, Filesystems

### 3. Checksum & Hash Algorithms
- **CRC32**: Error detection
- **MD5**: File verification
- **SHA256**: Cryptographic hashing
- **Usage**: Storage, Network, Security, Filesystems

### 4. Encoding Algorithms
- **Base64**: Binary-to-text encoding
- **Hex**: Hexadecimal encoding/decoding
- **URL Encoding**: URL-safe encoding
- **Usage**: Network protocols, IPC, Filesystems

### 5. Parsing Algorithms
- **INI**: Configuration file parsing
- **JSON**: Data format parsing
- **ELF**: Executable format parsing
- **PE**: Windows executable parsing
- **Usage**: Loader, Configuration, Debug tools

### 6. Data Structure Algorithms
- **Priority Queues**: Scheduling and ordering
- **Bloom Filters**: Probabilistic membership
- **LRU Caches**: Memory management
- **Usage**: All kernel domains

## Implementation Guidelines

### File Organization
```
Include/LibFK/Algorithms/
├── Archive/
│   ├── tar.h
│   ├── zip.h
│   └── gzip.h
├── Compression/
│   ├── lz4.h
│   ├── zlib.h
│   └── deflate.h
├── Checksum/
│   ├── crc32.h
│   ├── md5.h
│   └── sha256.h
├── Encoding/
│   ├── base64.h
│   ├── hex.h
│   └── url_encoding.h
├── Parsing/
│   ├── ini.h
│   ├── json.h
│   ├── elf.h
│   └── pe.h
└── DataStructures/
    ├── priority_queue.h
    ├── bloom_filter.h
    └── lru_cache.h

Src/LibFK/Algorithms/
├── Archive/
│   ├── tar.cpp
│   ├── zip.cpp
│   └── gzip.cpp
├── Compression/
│   ├── lz4.cpp
│   ├── zlib.cpp
│   └── deflate.cpp
├── Checksum/
│   ├── crc32.cpp
│   ├── md5.cpp
│   └── sha256.cpp
├── Encoding/
│   ├── base64.cpp
│   ├── hex.cpp
│   └── url_encoding.cpp
├── Parsing/
│   ├── ini.cpp
│   ├── json.cpp
│   ├── elf.cpp
│   └── pe.cpp
└── DataStructures/
    ├── priority_queue.cpp
    ├── bloom_filter.cpp
    └── lru_cache.cpp
```

### API Design Principles

1. **Domain-Agnostic**: Algorithms must not depend on specific kernel domains
2. **LibC Only**: Use only LibC and other LibFK algorithms
3. **Result-Based**: Return `Result<T, Error>` for fallible operations
4. **Memory Safe**: Use LibFK memory management, no raw pointers
5. **Template-Friendly**: Use templates for generic implementations

### Example Implementation

```cpp
// Include/LibFK/Algorithms/Archive/tar.h
namespace fk::Algorithms::Archive {
    class Tar {
    public:
        struct Header {
            // TAR header fields
        };
        
        static Result<fk::Vector<uint8_t>, Error> create_archive(const fk::Vector<fk::String>& files);
        static Result<void, Error> extract_archive(const uint8_t* data, size_t size, const fk::String& output_path);
        static Result<bool, Error> validate_archive(const uint8_t* data, size_t size);
    };
}

// Usage in domains
auto result = fk::Algorithms::Archive::Tar::extract_archive(archive_data, archive_size, "/tmp");
if (result.is_error()) {
    // Handle error
}
```

## Migration Process

### 1. Identification
- Search for duplicate algorithm implementations across domains
- Identify commonly used algorithms
- Catalog existing implementations

### 2. Consolidation
- Extract best implementation from existing duplicates
- Generalize for domain-agnostic use
- Move to `LibFK/Algorithms/` with proper categorization

### 3. Integration
- Replace domain-specific implementations with LibFK calls
- Update all include paths
- Ensure compilation across all domains

### 4. Testing
- Create comprehensive test suite for consolidated algorithms
- Test across all use cases from different domains
- Performance regression testing

### 5. Documentation
- Update algorithm documentation
- Document usage patterns
- Add migration guide for developers

## Benefits

### For Developers
- **Single Source of Truth**: One implementation to learn and use
- **Consistent API**: Same interface across all domains
- **Reduced Bugs**: One place to fix bugs
- **Better Performance**: Shared optimizations benefit all

### For the Project
- **Reduced Code Size**: Eliminate duplication
- **Easier Maintenance**: Centralized algorithm updates
- **Better Testing**: One comprehensive test suite
- **Consistency**: Same behavior across all components

## Enforcement

### Automated Validation
- GEMINI validator checks for algorithm duplication
- CI/CD prevents merging duplicate implementations
- Automated suggestions for consolidation

### Code Review Guidelines
- Review new algorithm implementations for consolidation opportunities
- Require justification for domain-specific algorithms
- Ensure proper use of consolidated algorithms

## Exception Process

Algorithms can remain domain-specific only if:

1. **Domain-Specific Requirements**: Algorithm has unique requirements for that domain
2. **Performance Critical**: Domain-specific implementation provides significant performance benefits
3. **Hardware Dependencies**: Algorithm depends on specific hardware features
4. **Legacy Compatibility**: Required for compatibility with existing interfaces

Exceptions must be documented and approved by architectural review.

---

**This policy ensures maximum code reuse while maintaining domain-specific flexibility when truly necessary.**