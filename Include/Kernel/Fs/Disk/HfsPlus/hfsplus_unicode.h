#pragma once

#include <Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h>
#include <LibFK/Types/types.h>

namespace fkernel {

// Convert a big-endian HFSUniStr255 to a null-terminated UTF-8 string.
// Only handles the Basic Multilingual Plane (UCS-2 range U+0000–U+FFFF).
// Returns the number of bytes written (excluding null terminator), or 0 on
// error.  dst_size must be at least 3*str.length + 1 bytes.
size_t hfsplus_unicode_to_utf8(const HFSUniStr255& str, char* dst, size_t dst_size);

// Convert a null-terminated UTF-8 string to HFSUniStr255 (big-endian UCS-2).
// Returns true on success.  Fails if the input contains codepoints above U+FFFF
// or if it exceeds 255 characters.
bool hfsplus_utf8_to_unicode(const char* src, HFSUniStr255& out);

// Case-insensitive comparison of two HFSUniStr255 values using the 256-entry
// HFS+ case-folding table.  Returns negative, zero, or positive, like strcmp.
int hfsplus_unicode_cmp_ci(const HFSUniStr255& a, const HFSUniStr255& b);

// Case-sensitive comparison (binary UCS-2 compare).
int hfsplus_unicode_cmp_cs(const HFSUniStr255& a, const HFSUniStr255& b);

} // namespace fkernel
