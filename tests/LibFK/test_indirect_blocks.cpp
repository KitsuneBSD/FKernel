#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/indirect_blocks.h>

using fk::core::Error;

namespace {

// Simulated disk of index blocks. Each block holds 4 pointers (nindir=4).
// Only the blocks referenced by the fixture below are populated.
uint32_t s_index[512][4];

// Direct roots: 12 direct + 3 indirect roots.
uint32_t s_roots[15];

void setup() {
    for (int b = 0; b < 512; ++b)
        for (int i = 0; i < 4; ++i)
            s_index[b][i] = 0;

    for (int i = 0; i < 15; ++i) s_roots[i] = 0;

    // Direct blocks 0..3 map to 200..203
    for (int i = 0; i < 4; ++i) s_roots[i] = static_cast<uint32_t>(200 + i);

    // Single indirect root: block 1 (which holds 100..103)
    s_roots[12] = 1;
    s_index[1][0] = 100;
    s_index[1][1] = 101;
    s_index[1][2] = 102;
    s_index[1][3] = 103;

    // Double indirect root: block 2 (holds 108..111), each is an index block
    s_roots[13] = 2;
    s_index[2][0] = 108;
    s_index[2][1] = 109;
    s_index[2][2] = 110;
    s_index[2][3] = 111;
    // 108 holds 300..303, 109 holds 304..307
    s_index[108][0] = 300;
    s_index[108][1] = 301;
    s_index[108][2] = 302;
    s_index[108][3] = 303;
    s_index[109][0] = 304;
    s_index[109][1] = 305;
    s_index[109][2] = 306;
    s_index[109][3] = 307;

    // Triple indirect root: block 3 (holds 112..115)
    s_roots[14] = 3;
    s_index[3][0] = 112;
    s_index[3][1] = 113;
    s_index[3][2] = 114;
    s_index[3][3] = 115;
    // 112 is an index block → 400..403
    s_index[112][0] = 400;
    s_index[112][1] = 401;
    s_index[112][2] = 402;
    s_index[112][3] = 403;
    // 400 is an index block → 500..503
    s_index[400][0] = 500;
    s_index[400][1] = 501;
    s_index[400][2] = 502;
    s_index[400][3] = 503;
}

fk::core::Result<uint32_t, Error> resolve_ptr(uint32_t block, uint32_t index) {
    if (block == 0) return Error::InvalidData;
    if (block >= 512 || index >= 4) return Error::InvalidData;
    return s_index[block][index];
}

} // namespace

static const char* test_direct() {
    setup();
    auto r0 = fk::algorithms::resolve_indirect<uint32_t>(0, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT(r0.is_ok(), "direct block 0");
    TEST_ASSERT_EQ((uint64_t)r0.value(), (uint64_t)200, "direct 0 -> 200");
    auto r3 = fk::algorithms::resolve_indirect<uint32_t>(3, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT_EQ((uint64_t)r3.value(), (uint64_t)203, "direct 3 -> 203");
    return nullptr;
}

static const char* test_single_indirect() {
    setup();
    // logical 12 → first single-indirect slot: index[1][0] = 100
    auto r = fk::algorithms::resolve_indirect<uint32_t>(12, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT(r.is_ok(), "single indirect logical 12");
    TEST_ASSERT_EQ((uint64_t)r.value(), (uint64_t)100, "logical 12 -> 100");
    auto r15 = fk::algorithms::resolve_indirect<uint32_t>(15, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT_EQ((uint64_t)r15.value(), (uint64_t)103, "logical 15 -> 103");
    return nullptr;
}

static const char* test_double_indirect() {
    setup();
    // logical 16 → l1=0,l2=0 → roots[5]=2 → index[2][0]=108 → index[108][0]=300
    auto r = fk::algorithms::resolve_indirect<uint32_t>(16, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT(r.is_ok(), "double indirect logical 16");
    TEST_ASSERT_EQ((uint64_t)r.value(), (uint64_t)300, "logical 16 -> 300");
    // logical 21 → l1=1,l2=1 → 109 → index[109][1]=305
    auto r21 = fk::algorithms::resolve_indirect<uint32_t>(21, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT_EQ((uint64_t)r21.value(), (uint64_t)305, "logical 21 -> 305");
    return nullptr;
}

static const char* test_triple_indirect() {
    setup();
    // logical 28 = 12 + 4 + 16 - ... = after direct(4)+single(4)+double(16) = index 28
    // logical 28 → idx=16 after double → triple: l1=1,l2=0,l3=0
    //   roots[6]=3 → index[3][1]=101... wait: index[3]={112,113,114,115}; [1]=113
    // Actually logical 28: logical-12=16, -nindir(4)=12, -sq(16) → negative → stays in double? No.
    // 28-12=16; 16<4? no; 16-4=12; 12<16? yes → double l1=12/4=3,l2=12%4=0
    //   roots[5]=2 → index[2][3]=111 → index[111]... but we didn't set 111.
    // Use logical 27 (double boundary): 27-12=15; 15>=4; 15-4=11; 11<16 → double l1=2,l2=3
    //   index[2][2]=110, index[110][3] unset → 0. Skip; test triple at logical 28+.
    // logical 28 = 12+4+4+16? no. Addressable single: [12..15], double: [16..31].
    // triple starts at 32: 32-12=20; 20-4=16; 16-16=0; 0<16 → l1=0,l2=0,l3=0
    //   roots[6]=3 → index[3][0]=112 → index[112][0]=400 → index[400][0]=500
    auto r = fk::algorithms::resolve_indirect<uint32_t>(32, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT(r.is_ok(), "triple indirect logical 32");
    TEST_ASSERT_EQ((uint64_t)r.value(), (uint64_t)500, "logical 32 -> 500");
    return nullptr;
}

static const char* test_out_of_range() {
    setup();
    // 12 + 4 + 16 + 64 = 96 → beyond triple
    auto r = fk::algorithms::resolve_indirect<uint32_t>(96, 4, s_roots, 12, 15, resolve_ptr);
    TEST_ASSERT(r.is_error(), "out-of-range logical should error");
    return nullptr;
}

static const char* test_64bit_pointer() {
    setup();
    uint64_t roots64[15];
    for (int i = 0; i < 15; ++i) roots64[i] = s_roots[i];
    auto resolve64 = [](uint64_t block, uint32_t index) -> fk::core::Result<uint64_t, Error> {
        return static_cast<uint64_t>(resolve_ptr(static_cast<uint32_t>(block), index).value());
    };
    auto r = fk::algorithms::resolve_indirect<uint64_t>(16, 4, roots64, 12, 15, resolve64);
    TEST_ASSERT(r.is_ok(), "64-bit double indirect");
    TEST_ASSERT_EQ(r.value(), (uint64_t)300, "64-bit logical 16 -> 300");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"direct blocks", test_direct},
    {"single indirect", test_single_indirect},
    {"double indirect", test_double_indirect},
    {"triple indirect", test_triple_indirect},
    {"out of range", test_out_of_range},
    {"64-bit pointers", test_64bit_pointer},
};

int run_libfk_indirect_blocks_tests() {
    return run_tests("IndirectBlocks", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
