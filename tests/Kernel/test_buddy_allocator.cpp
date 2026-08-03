#include <tests/test_framework.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Utilities/memory.h>

// Host-side tests for BuddyAllocator::invalidate_page (M3) and the absolute
// order contract (M1).  BuddyAllocator stores FreeBlock nodes at
// (KERNEL_VIRT_BASE + phys), so we back a fake "physical" range with a real
// buffer and use unsigned-wrap arithmetic (same trick as test_buddy_state):
//   KERNEL_VIRT_BASE + fake_phys(&g_mem[i]) == &g_mem[i].
// The buffer is 64 KiB-aligned so initialize_from_bitmap exposes the whole
// range as a single order-16 block (order_to_size(16) == 64 KiB).

static constexpr size_t TEST_PAGES = 16;         // 16 x 4 KiB = 64 KiB
static constexpr size_t TEST_ORDER = 16;         // order covering the whole range
static constexpr size_t TEST_BYTES = TEST_PAGES * BUDDY_PAGE_SIZE;

alignas(65536) static uint8_t g_mem[TEST_BYTES];
static uint64_t g_bitmap_storage[1];

static uintptr_t fake_phys(void* v) {
  return reinterpret_cast<uintptr_t>(v) - KERNEL_VIRT_BASE;
}

static uintptr_t zone_base() {
  return fake_phys(g_mem);
}

static BuddyAllocator make_fresh_allocator() {
  fk::memory::set(g_mem, 0, sizeof(g_mem));
  BuddyAllocator buddy;
  buddy.add_range(zone_base(), TEST_BYTES);
  fk::containers::Bitmap<uint64_t> bitmap(g_bitmap_storage, TEST_PAGES);
  buddy.initialize_from_bitmap(bitmap, zone_base());
  return buddy;
}

static const char* test_absolute_order_contract() {
  TEST_ASSERT_EQ(4096u, order_to_size(MIN_ORDER), "order 12 -> 4096 bytes");
  TEST_ASSERT_EQ(8192u, order_to_size(13), "order 13 -> 8192 bytes");
  TEST_ASSERT_EQ(12u, size_to_order(4096), "4 KiB -> order 12");
  TEST_ASSERT_EQ(13u, size_to_order(8192), "8 KiB -> order 13");
  TEST_ASSERT_EQ(13u, size_to_order(5000), "5 KiB rounds up to order 13");
  return nullptr;
}

static const char* test_fresh_allocator_hands_out_whole_range() {
  BuddyAllocator buddy = make_fresh_allocator();
  TEST_ASSERT((void*)zone_base() == buddy.alloc(TEST_ORDER),
              "fresh allocator exposes the range as one order-16 block");
  return nullptr;
}

static const char* test_invalidate_internal_page_blocks_big_alloc() {
  BuddyAllocator buddy = make_fresh_allocator();
  uintptr_t page1 = zone_base() + BUDDY_PAGE_SIZE;

  // page1 sits in the middle of the order-16 block.  After invalidating it,
  // the block must be split so it can no longer be handed out whole.
  buddy.invalidate_page(page1);
  TEST_ASSERT(nullptr == buddy.alloc(TEST_ORDER),
              "block containing an invalidated page must not be allocatable");
  return nullptr;
}

static const char* test_invalidate_internal_page_keeps_others_allocatable() {
  BuddyAllocator buddy = make_fresh_allocator();
  uintptr_t page1 = zone_base() + BUDDY_PAGE_SIZE;
  buddy.invalidate_page(page1);

  uintptr_t got[TEST_PAGES];
  size_t count = 0;
  for (;;) {
    void* blk = buddy.alloc(MIN_ORDER);
    if (!blk) break;
    TEST_ASSERT(count < TEST_PAGES, "never hand out more than total page count");
    got[count++] = reinterpret_cast<uintptr_t>(blk);
  }

  TEST_ASSERT_EQ(TEST_PAGES - 1, count, "15 pages remain after one invalidated");

  for (size_t i = 0; i < count; i++) {
    TEST_ASSERT(got[i] != page1, "invalidated page must never be handed out");
    for (size_t j = i + 1; j < count; j++)
      TEST_ASSERT(got[i] != got[j], "no double allocation");
  }
  return nullptr;
}

static const char* test_invalidate_then_free_merges_back() {
  BuddyAllocator buddy = make_fresh_allocator();
  uintptr_t page1 = zone_base() + BUDDY_PAGE_SIZE;

  // invalidate_page splits the big block into a hole; freeing the page merges
  // back up through its order-12 buddy (base) and the sibling halves
  // (base+8192, base+16384, base+32768) into the original order-16 block.
  buddy.invalidate_page(page1);
  buddy.free(reinterpret_cast<void*>(page1), MIN_ORDER);

  TEST_ASSERT((void*)zone_base() == buddy.alloc(TEST_ORDER),
              "freeing the invalidated page must re-merge the whole block");
  return nullptr;
}

static test_case_t s_tests[] = {
    {"absolute_order_contract",               test_absolute_order_contract},
    {"fresh_allocator_hands_out_whole_range", test_fresh_allocator_hands_out_whole_range},
    {"invalidate_internal_blocks_big_alloc",  test_invalidate_internal_page_blocks_big_alloc},
    {"invalidate_keeps_others_allocatable",   test_invalidate_internal_page_keeps_others_allocatable},
    {"invalidate_then_free_merges_back",      test_invalidate_then_free_merges_back},
};

int run_kernel_buddy_allocator_tests() {
  return run_tests("Kernel::BuddyAllocator",
                   s_tests,
                   sizeof(s_tests) / sizeof(s_tests[0]));
}
