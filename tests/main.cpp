#include <tests/test_framework.h>

// Forward declarations of test runners
int run_libfk_traits_tests();
int run_libc_string_tests();
int run_libfk_container_tests();
int run_libfk_container_advanced_tests();
int run_libfk_smart_pointer_tests();
int run_libfk_text_tests();
int run_libfk_multi_container_tests();
int run_libfk_tuple_tests();
int run_libfk_stack_queue_staticvec_tests();
int run_libfk_nonnull_weak_bump_tests();
int run_libfk_lock_rank_format_tests();
int run_libfk_stringbuilder_tests();
int run_libfk_bitmap_unordered_set_tests();
int run_libfk_algorithm_tests();
int run_libfk_string_view_tests();
int run_libfk_fixed_string_tests();
int run_libfk_byte_checksum_tests();
int run_libfk_byte_order_tests();
int run_libfk_crc32_tests();
int run_libfk_scatter_io_tests();
int run_libfk_indirect_blocks_tests();
int run_libfk_bitmap_tests();
int run_libfk_path_tests();
int run_libfk_time_math_tests();

// Kernel subsystem tests (host-side, mocked hardware)
int run_kernel_file_lock_tests();
int run_kernel_cspace_tests();
int run_kernel_elf_header_tests();
int run_kernel_slab_free_list_tests();
int run_kernel_buddy_state_tests();
int run_kernel_zone_allocator_tests();
int run_kernel_qos_tests();
int run_kernel_dentry_tests();
int run_kernel_buddy_allocator_tests();
int run_kernel_slab_allocator_tests();

int main() {
    int failed = 0;

    failed += run_libfk_traits_tests();
    failed += run_libc_string_tests();
    failed += run_libfk_container_tests();
    failed += run_libfk_container_advanced_tests();
    failed += run_libfk_smart_pointer_tests();
    failed += run_libfk_text_tests();
    failed += run_libfk_multi_container_tests();
    failed += run_libfk_tuple_tests();
    failed += run_libfk_stack_queue_staticvec_tests();
    failed += run_libfk_nonnull_weak_bump_tests();
    failed += run_libfk_lock_rank_format_tests();
    failed += run_libfk_stringbuilder_tests();
    failed += run_libfk_bitmap_unordered_set_tests();
    failed += run_libfk_algorithm_tests();
    failed += run_libfk_string_view_tests();
    failed += run_libfk_fixed_string_tests();
    failed += run_libfk_byte_checksum_tests();
    failed += run_libfk_byte_order_tests();
    failed += run_libfk_crc32_tests();
    failed += run_libfk_scatter_io_tests();
    failed += run_libfk_indirect_blocks_tests();
    failed += run_libfk_bitmap_tests();
    failed += run_libfk_path_tests();
    failed += run_libfk_time_math_tests();

    // Kernel subsystem tests
    failed += run_kernel_file_lock_tests();
    failed += run_kernel_cspace_tests();
    failed += run_kernel_elf_header_tests();
    failed += run_kernel_slab_free_list_tests();
    failed += run_kernel_buddy_state_tests();
    failed += run_kernel_zone_allocator_tests();
    failed += run_kernel_qos_tests();
    failed += run_kernel_dentry_tests();
    failed += run_kernel_buddy_allocator_tests();
    failed += run_kernel_slab_allocator_tests();

    if (failed == 0) {
        TEST_LOG("\n>>> SUMMARY: ALL TEST SUITES PASSED!\n");
    } else {
        TEST_LOG("\n>>> SUMMARY: %d TOTAL TESTS FAILED!\n", failed);
    }
    
    return failed != 0;
}
