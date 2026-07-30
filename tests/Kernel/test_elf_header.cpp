#include <tests/test_framework.h>
#include <Kernel/Loader/elf_validation.h>
#include <LibFK/Types/types.h>
#include <string.h>

using fkernel::Elf64_Ehdr;
using fkernel::elf_check_header;

// Build a minimal valid x86_64 ELF64 header
static Elf64_Ehdr make_valid_header() {
    Elf64_Ehdr h;
    memset(&h, 0, sizeof(h));
    h.e_ident[0] = 0x7f;
    h.e_ident[1] = 'E';
    h.e_ident[2] = 'L';
    h.e_ident[3] = 'F';
    h.e_ident[4] = 2;               // ELFCLASS64
    h.e_ident[5] = ELFDATA2LSB;     // little-endian
    h.e_type      = ET_EXEC;
    h.e_machine   = EM_X86_64;
    h.e_version   = 1;
    h.e_ehsize    = sizeof(Elf64_Ehdr);
    h.e_phentsize = 56;
    h.e_phnum     = 0;
    h.e_phoff     = 0;
    return h;
}

static const char* test_valid_header() {
    auto h = make_valid_header();
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_ok(), "valid header must pass validation");
    return nullptr;
}

static const char* test_valid_dyn() {
    auto h = make_valid_header();
    h.e_type = ET_DYN;
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_ok(), "ET_DYN header must pass validation");
    return nullptr;
}

static const char* test_wrong_magic_byte0() {
    auto h = make_valid_header();
    h.e_ident[0] = 0x00; // wrong first byte
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "wrong magic[0] must fail");
    return nullptr;
}

static const char* test_wrong_magic_e() {
    auto h = make_valid_header();
    h.e_ident[1] = 'X'; // wrong 'E'
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "wrong magic[1] must fail");
    return nullptr;
}

static const char* test_wrong_magic_l() {
    auto h = make_valid_header();
    h.e_ident[2] = 'X'; // wrong first 'L'
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "wrong magic[2] must fail");
    return nullptr;
}

static const char* test_wrong_magic_f() {
    auto h = make_valid_header();
    h.e_ident[3] = 'X'; // wrong 'F'
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "wrong magic[3] must fail");
    return nullptr;
}

static const char* test_big_endian_rejected() {
    auto h = make_valid_header();
    h.e_ident[5] = ELFDATA2MSB; // big-endian
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "big-endian ELF must be rejected");
    return nullptr;
}

static const char* test_32bit_class_rejected() {
    auto h = make_valid_header();
    h.e_ident[4] = 1; // ELFCLASS32
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "32-bit class ELF must be rejected");
    return nullptr;
}

static const char* test_wrong_machine() {
    auto h = make_valid_header();
    h.e_machine = 3; // EM_386
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "EM_386 machine must be rejected");
    return nullptr;
}

static const char* test_phnum_at_limit() {
    auto h = make_valid_header();
    h.e_phnum  = ELF_MAX_PHNUM;
    h.e_phoff  = sizeof(Elf64_Ehdr);
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_ok(), "e_phnum == ELF_MAX_PHNUM must be accepted");
    return nullptr;
}

static const char* test_phnum_exceeds_limit() {
    auto h = make_valid_header();
    h.e_phnum  = ELF_MAX_PHNUM + 1;
    h.e_phoff  = sizeof(Elf64_Ehdr);
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "e_phnum > ELF_MAX_PHNUM must be rejected");
    return nullptr;
}

static const char* test_phoff_overlaps_header() {
    auto h = make_valid_header();
    h.e_phnum  = 2;
    h.e_phoff  = sizeof(Elf64_Ehdr) - 1; // overlaps ELF header
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_error(), "e_phoff inside ELF header must be rejected");
    return nullptr;
}

static const char* test_phoff_zero_with_no_phdrs() {
    auto h = make_valid_header();
    h.e_phnum = 0;
    h.e_phoff = 0; // zero offset is OK when there are no phdrs
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_ok(), "e_phoff=0 with e_phnum=0 must be accepted");
    return nullptr;
}

static const char* test_phoff_just_past_header() {
    auto h = make_valid_header();
    h.e_phnum  = 1;
    h.e_phoff  = sizeof(Elf64_Ehdr); // exactly at end of ELF header
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_ok(), "e_phoff == sizeof(Ehdr) must be accepted");
    return nullptr;
}

static const char* test_check_returns_copy() {
    auto h = make_valid_header();
    h.e_entry = 0xdeadbeef;
    auto res = elf_check_header(h);
    TEST_ASSERT(res.is_ok(), "valid header must succeed");
    TEST_ASSERT_EQ((long)0xdeadbeef, (long)res.value().e_entry, "returned header must preserve e_entry");
    return nullptr;
}

int run_kernel_elf_header_tests() {
    static const test_case_t tests[] = {
        {"valid_exec_header",           test_valid_header},
        {"valid_dyn_header",            test_valid_dyn},
        {"wrong_magic_byte0",           test_wrong_magic_byte0},
        {"wrong_magic_e",               test_wrong_magic_e},
        {"wrong_magic_l",               test_wrong_magic_l},
        {"wrong_magic_f",               test_wrong_magic_f},
        {"big_endian_rejected",         test_big_endian_rejected},
        {"32bit_class_rejected",        test_32bit_class_rejected},
        {"wrong_machine",               test_wrong_machine},
        {"phnum_at_limit",              test_phnum_at_limit},
        {"phnum_exceeds_limit",         test_phnum_exceeds_limit},
        {"phoff_overlaps_header",       test_phoff_overlaps_header},
        {"phoff_zero_no_phdrs",         test_phoff_zero_with_no_phdrs},
        {"phoff_just_past_header",      test_phoff_just_past_header},
        {"check_returns_header_copy",   test_check_returns_copy},
    };
    return run_tests("Kernel::ElfHeaderValidation", tests,
                     sizeof(tests) / sizeof(tests[0]));
}
