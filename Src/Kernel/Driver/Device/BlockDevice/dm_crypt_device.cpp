#include <LibFK/Algorithms/Crypto/sha1.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Utilities/memory.h>
#include <Kernel/Driver/Device/BlockDevice/dm_crypt_device.h>

namespace fkernel {

// Read a big-endian uint32_t stored in a raw LUKS header field.
// After memcpy of disk bytes on a little-endian host, uint32_t fields are byte-swapped.
static uint32_t from_be32(uint32_t v) { return __builtin_bswap32(v); }
static uint16_t from_be16(uint16_t v) { return __builtin_bswap16(v); }

// LUKS AF diffuse: replace each SHA1-sized chunk with SHA1(be32(i) || chunk).
// This spreads entropy across the accumulator block (per LUKS spec §5.2).
static void luks_diffuse(uint8_t* block, uint32_t keylen) {
    constexpr uint32_t D = 20; // SHA1 output size
    uint8_t input[4 + 20];    // IV (4) + one chunk (up to 20)
    uint8_t hash[D];
    uint32_t i = 0;
    for (uint32_t off = 0; off < keylen; off += D, ++i) {
        uint32_t chunk = (keylen - off < D) ? (keylen - off) : D;
        // big-endian counter
        input[0] = (i >> 24) & 0xFF; input[1] = (i >> 16) & 0xFF;
        input[2] = (i >>  8) & 0xFF; input[3] =  i        & 0xFF;
        fk::memory::copy(input + 4, block + off, chunk);
        fk::algorithms::sha1(input, 4 + chunk, hash);
        fk::memory::copy(block + off, hash, chunk);
    }
}

// AF-merge (LUKS spec §5.2): reconstruct master key from `stripes` segments of `keylen` bytes.
static void af_merge(const uint8_t* src, uint32_t stripes, uint32_t keylen, uint8_t* out) {
    uint8_t acc[64] = {};
    for (uint32_t s = 0; s < stripes - 1; ++s) {
        const uint8_t* stripe = src + (size_t)s * keylen;
        for (uint32_t j = 0; j < keylen; ++j) acc[j] ^= stripe[j];
        luks_diffuse(acc, keylen);
    }
    const uint8_t* last = src + (size_t)(stripes - 1) * keylen;
    for (uint32_t j = 0; j < keylen; ++j) out[j] = acc[j] ^ last[j];
}

fk::core::Result<void, fk::core::Error> DmCryptDevice::read_header(LuksHeader& hdr) {
    if (m_children.is_empty()) return fk::core::Error::DeviceError;
    auto& dev = *m_children[0];
    uint8_t buf[512];
    TRY(dev.read_sectors(0, 1, buf));
    fk::memory::copy(&hdr, buf, sizeof(LuksHeader));
    return {};
}

fk::core::Result<bool, fk::core::Error>
DmCryptDevice::try_key_slot(const LuksHeader& hdr, uint32_t slot,
                             const uint8_t* pw, size_t pwlen) {
    const LuksKeySlot& ks = hdr.key_slots[slot];
    if (from_be32(ks.active) != LUKS_ACTIVE) return false;

    uint32_t keylen   = from_be32(hdr.key_bytes);
    uint32_t stripes  = from_be32(ks.stripes);
    uint32_t km_off   = from_be32(ks.key_material_offset);
    uint32_t iters    = from_be32(ks.iterations);

    // Only AES-256-XTS (64-byte master key) supported.
    if (keylen != 64) {
        fk::algorithms::kwarn("DM_CRYPT", "Unsupported key size %u (only 64-byte AES-256-XTS)", keylen);
        return false;
    }

    // Derive the 64-byte slot key from the password via PBKDF2-HMAC-SHA1.
    uint8_t slot_key[64] = {};
    fk::algorithms::pbkdf2_hmac_sha1(pw, pwlen, ks.salt, 32, iters, slot_key, keylen);

    // Read encrypted key material (stripes * keylen bytes, rounded up to sectors).
    uint32_t km_bytes   = stripes * keylen;
    uint32_t km_sectors = (km_bytes + 511) / 512;
    uint8_t* km_buf = static_cast<uint8_t*>(kmalloc(km_sectors * 512));
    if (!km_buf) return fk::core::Error::OutOfMemory;

    auto read_res = m_children[0]->read_sectors(km_off, km_sectors, km_buf);
    if (read_res.is_error()) { kfree(km_buf); return read_res.error(); }

    // Decrypt key material with the slot AES-XTS key.
    fk::algorithms::AesXtsKey slot_xts;
    fk::algorithms::aes_xts_key_init(slot_key, &slot_xts);
    size_t blocks = km_bytes / 512;
    for (size_t i = 0; i < blocks; ++i)
        fk::algorithms::aes_xts_decrypt(&slot_xts, i, km_buf + i * 512, 512);

    // AF-merge to recover the master key candidate.
    uint8_t mk[64] = {};
    af_merge(km_buf, stripes, keylen, mk);
    kfree(km_buf);

    // Verify: PBKDF2(mk, mk_digest_salt, mk_digest_iter) must match mk_digest.
    uint8_t verify[20];
    fk::algorithms::pbkdf2_hmac_sha1(mk, keylen, hdr.mk_digest_salt, 32,
                                     from_be32(hdr.mk_digest_iter), verify, 20);
    if (fk::memory::compare(verify, hdr.mk_digest, 20) != 0) return false;

    fk::algorithms::aes_xts_key_init(mk, &m_xts);
    return true;
}

fk::core::Result<void, fk::core::Error>
DmCryptDevice::unlock(const uint8_t* pw, size_t pwlen) {
    LuksHeader hdr;
    TRY(read_header(hdr));

    // Validate magic
    static const uint8_t magic[6] = {'L','U','K','S',0xba,0xbe};
    if (fk::memory::compare(hdr.magic, magic, 6) != 0 || from_be16(hdr.version) != 1)
        return fk::core::Error::InvalidData;

    m_payload_offset = from_be32(hdr.payload_offset);
    m_sector_size    = 512;

    for (uint32_t i = 0; i < LUKS_KEY_SLOTS; ++i) {
        auto res = try_key_slot(hdr, i, pw, pwlen);
        if (res.is_error()) continue;
        if (res.value()) {
            m_unlocked = true;
            fk::algorithms::klog("DM_CRYPT", "Unlocked via key slot %u", i);
            return {};
        }
    }
    return fk::core::Error::PermissionDenied;
}

fk::core::Result<size_t, fk::core::Error>
DmCryptDevice::read_sectors(uint64_t start, size_t count, uint8_t* buf) {
    if (!m_unlocked || m_children.is_empty()) return fk::core::Error::DeviceError;
    uint64_t phys = m_payload_offset + start;
    TRY(m_children[0]->read_sectors(phys, count, buf));
    for (size_t i = 0; i < count; ++i)
        fk::algorithms::aes_xts_decrypt(&m_xts, start + i, buf + i * 512, 512);
    return count;
}

fk::core::Result<size_t, fk::core::Error>
DmCryptDevice::write_sectors(uint64_t start, size_t count, const uint8_t* buf) {
    if (!m_unlocked || m_children.is_empty()) return fk::core::Error::DeviceError;
    uint8_t* tmp = static_cast<uint8_t*>(kmalloc(count * 512));
    if (!tmp) return fk::core::Error::OutOfMemory;
    fk::memory::copy(tmp, buf, count * 512);
    for (size_t i = 0; i < count; ++i)
        fk::algorithms::aes_xts_encrypt(&m_xts, start + i, tmp + i * 512, 512);
    uint64_t phys = m_payload_offset + start;
    auto res = m_children[0]->write_sectors(phys, count, tmp);
    kfree(tmp);
    return res;
}

SectorCount DmCryptDevice::sector_count() const {
    if (m_children.is_empty()) return SectorCount(0);
    auto cnt = m_children[0]->sector_count().value();
    if (cnt < m_payload_offset) return SectorCount(0);
    return SectorCount(cnt - m_payload_offset);
}

} // namespace fkernel
