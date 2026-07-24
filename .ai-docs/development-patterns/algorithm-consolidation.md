# Algorithm Consolidation Policy

## Overview

This document establishes the policy for consolidating known algorithms used across multiple FKernel domains into `LibFK/Algorithms/` for maximum reusability and maintainability.

## Policy Statement

**All known algorithms used across multiple kernel domains MUST be consolidated in `LibFK/Algorithms/` rather than being duplicated in domain-specific implementations.**

## Implemented Algorithms

The following algorithms have been consolidated into `LibFK/Algorithms/`:

| File | Content | Status |
|------|---------|--------|
| `binary_search.h` | `lower_bound`, `upper_bound` | Done |
| `byte_checksum.h` | ACPI table byte-sum validation | Done |
| `byte_order.h` | `htons`, `htonl`, `ntohs`, `ntohl` | Done |
| `container_algorithms.h` | `find_if`, `find_and_remove`, `swap_remove`, `insert_if_absent` | Done |
| `crc32.h` | CRC32 checksum | Done |
| `djb2.h` | DJB2 hash function | Done |
| `fat_name.h` | 8.3 FAT name formatting (trim + concat) | Done |
| `gather.h` | Gather copy from iovec | Done |
| `internet_checksum.h` | RFC 1071 internet checksum | Done |
| `log.h` | Kernel logging utilities | Done |
| `math.h` | `abs()`, `swap()` | Done |
| `string_algorithms.h` | Case-insensitive string compare | Done |

## Planned Algorithms

These are planned but not yet implemented:

| Category | Algorithm | Priority |
|----------|-----------|----------|
| Archive | TAR, ZIP, GZIP | Medium |
| Compression | LZ4, ZLIB, DEFLATE | Low |
| Checksum | MD5, SHA256 | Low |
| Encoding | Base64, Hex, URL | Low |
| Parsing | INI, JSON, ELF, PE | Low (ELF parser already in Loader) |
| Data Structures | Bloom Filter, LRU Cache | Low |

## Implementation Guidelines

### File Organization
```
Include/LibFK/Algorithms/
+-- (flat directory -- all headers at top level for simplicity)
```

### API Design Principles

1. **Domain-Agnostic**: Algorithms must not depend on specific kernel domains
2. **LibC Only**: Use only LibC and other LibFK algorithms
3. **Result-Based**: Return `Result<T, Error>` for fallible operations
4. **Memory Safe**: Use LibFK memory management, no raw pointers
5. **Template-Friendly**: Use templates for generic implementations

## Migration Process

### 1. Identification
- Search for duplicate algorithm implementations across domains
- Identify commonly used algorithms
- Catalog existing implementations

### 2. Consolidation
- Extract best implementation from existing duplicates
- Generalize for domain-agnostic use
- Move to `LibFK/Algorithms/` with proper naming

### 3. Integration
- Replace domain-specific implementations with LibFK calls
- Update all include paths
- Ensure compilation across all domains

### 4. Testing
- Create comprehensive test suite for consolidated algorithms
- Test across all use cases from different domains

### 5. Documentation
- Update algorithm documentation
- Document usage patterns

## Enforcement

- Code review should flag duplicated algorithm implementations
- New algorithm implementations should justify staying domain-specific
- Use existing LibFK algorithms where possible

## Exception Process

Algorithms can remain domain-specific only if:

1. **Domain-Specific Requirements**: Algorithm has unique requirements for that domain
2. **Performance Critical**: Domain-specific implementation provides significant performance benefits
3. **Hardware Dependencies**: Algorithm depends on specific hardware features
4. **Legacy Compatibility**: Required for compatibility with existing interfaces

Exceptions must be documented.
