#include <tests/test_framework.h>
#include <LibFK/Algorithms/Generic/path.h>
#include <LibFK/Utilities/memory.h>

using fk::algorithms::PathTokenizer;

static bool collect(const char* path, char out[][32], int max, int& count) {
    PathTokenizer tok(path, 100);
    const char* data;
    size_t len;
    count = 0;
    while (tok.next_component(data, len)) {
        if (count >= max) return false;
        for (size_t i = 0; i < len; ++i) out[count][i] = data[i];
        out[count][len] = '\0';
        count++;
    }
    return true;
}

static const char* test_simple_path() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("/usr/bin/ls", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 3, "three components");
    TEST_ASSERT_STR_EQ("usr", comps[0], "first component");
    TEST_ASSERT_STR_EQ("bin", comps[1], "second component");
    TEST_ASSERT_STR_EQ("ls", comps[2], "third component");
    return nullptr;
}

static const char* test_relative_path() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("a/b", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 2, "two components");
    TEST_ASSERT_STR_EQ("a", comps[0], "first");
    TEST_ASSERT_STR_EQ("b", comps[1], "second");
    return nullptr;
}

static const char* test_skips_duplicate_slashes() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("//a///b//", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 2, "duplicate slashes skipped");
    TEST_ASSERT_STR_EQ("a", comps[0], "first");
    TEST_ASSERT_STR_EQ("b", comps[1], "second");
    return nullptr;
}

static const char* test_leading_trailing_slash() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("/etc/", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 1, "one component");
    TEST_ASSERT_STR_EQ("etc", comps[0], "component");
    return nullptr;
}

static const char* test_empty_path() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 0, "no components");
    return nullptr;
}

static const char* test_root_only() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("/", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 0, "root yields nothing");
    return nullptr;
}

static const char* test_dotdot_components_yielded() {
    char comps[8][32];
    int n = 0;
    TEST_ASSERT(collect("a/../b", comps, 8, n), "collect ok");
    TEST_ASSERT_EQ(n, 3, "dot components yielded to caller");
    TEST_ASSERT_STR_EQ("a", comps[0], "first");
    TEST_ASSERT_STR_EQ("..", comps[1], "dotdot yielded");
    TEST_ASSERT_STR_EQ("b", comps[2], "third");
    return nullptr;
}

static const char* test_maxlen_bounds() {
    // "ab/cd" but maxlen=3 → only "ab" then cursor hits m_end
    PathTokenizer tok("ab/cd", 3);
    const char* data;
    size_t len;
    TEST_ASSERT(tok.next_component(data, len), "first component");
    TEST_ASSERT_EQ(len, (size_t)2, "component length 2");
    TEST_ASSERT(!tok.next_component(data, len), "no second component beyond maxlen");
    return nullptr;
}

static const char* test_find_last_slash() {
    const char* last = fk::memory::find_last("/a/b/c", '/', 10);
    TEST_ASSERT_NOT_NULL(last, "found slash");
    TEST_ASSERT_EQ((int)(last[1]), (int)'c', "points at last slash");
    return nullptr;
}

static const char* test_find_last_no_match() {
    const char* last = fk::memory::find_last("nopath", '/', 10);
    TEST_ASSERT(last == nullptr, "no slash returns null");
    return nullptr;
}

static const char* test_find_last_respects_maxlen() {
    // "/a/b" with maxlen 3 → scans only "/a/" → last slash is at index 2
    const char* p = "/a/b";
    const char* last = fk::memory::find_last(p, '/', 3);
    TEST_ASSERT_NOT_NULL(last, "found slash within maxlen");
    TEST_ASSERT(last == p + 2, "last slash in range is index 2");
    return nullptr;
}

static const char* test_find_last_respects_nul() {
    const char* last = fk::memory::find_last("ab/cd", '/', 100);
    TEST_ASSERT_NOT_NULL(last, "found slash");
    TEST_ASSERT_EQ((int)(last[1]), (int)'c', "last slash before NUL");
    return nullptr;
}

static const test_case_t s_tests[] = {
    {"simple path", test_simple_path},
    {"relative path", test_relative_path},
    {"skips duplicate slashes", test_skips_duplicate_slashes},
    {"leading trailing slash", test_leading_trailing_slash},
    {"empty path", test_empty_path},
    {"root only", test_root_only},
    {"dotdot components yielded", test_dotdot_components_yielded},
    {"maxlen bounds", test_maxlen_bounds},
    {"find_last slash", test_find_last_slash},
    {"find_last no match", test_find_last_no_match},
    {"find_last respects maxlen", test_find_last_respects_maxlen},
    {"find_last respects NUL", test_find_last_respects_nul},
};

int run_libfk_path_tests() {
    return run_tests("Path", s_tests, sizeof(s_tests) / sizeof(s_tests[0]));
}
