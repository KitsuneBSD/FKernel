#pragma once

#include <LibFK/Algorithms/djb2.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>
#include <LibFK/Container/string.h>
#include <LibFK/Container/string_view.h>
#include <LibFK/Container/fixed_string.h>
#include <LibFK/Container/array.h>
#include <LibFK/Container/span.h>
#include <LibFK/Container/static_vector.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Container/stack.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Memory/retain_ptr.h>
#include <LibFK/Tree/rb_tree.h>
#include <LibFK/Utilities/pair.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

/**
 * @brief Template traits for generic types.
 *
 * Provides auxiliary functions such as hashing and dumping
 * for different data types.
 *
 * @tparam T Data type
 */
template <typename T> struct Traits {};

/**
 * @brief Traits specialization for int
 */
template <> struct Traits<int> {
  /**
   * @brief Computes the hash of an integer using DJB2.
   *
   * @param i Integer value to hash
   * @return Hash value
   */
  static unsigned hash(int i) { return djb2(&i, sizeof(i)); }

  /**
   * @brief Prints the integer value.
   *
   * @param i Integer value to print
   */
  static void dump(int i) { kprintf("%d", i); }
};

/**
 * @brief Traits specialization for unsigned int
 */
template <> struct Traits<unsigned> {
  /**
   * @brief Computes the hash of an unsigned integer using DJB2.
   *
   * @param i Unsigned integer value to hash
   * @return Hash value
   */
  static unsigned hash(unsigned i) { return djb2(&i, sizeof(i)); }

  /**
   * @brief Prints the unsigned integer value.
   *
   * @param i Unsigned integer value to print
   */
  static void dump(unsigned i) { kprintf("%u", i); }
};

template <> struct Traits<String> {
  static unsigned hash(const String &s) { return djb2(s.c_str(), s.length()); }
  static void dump(const String &s) { kprintf("%s", s.c_str()); }
};

template <> struct Traits<StringView> {
  static unsigned hash(const StringView &sv) { return djb2(sv.data(), sv.size()); }
  static void dump(const StringView &sv) {
    if (sv.empty()) return;
    char buf[256];
    size_t write_len = sv.size() < sizeof(buf) ? sv.size() : sizeof(buf) - 1;
    memcpy(buf, sv.data(), write_len);
    buf[write_len] = '\0';
    kprintf("%s", buf);
    if (write_len < sv.size()) {
        kprintf("...");
    }
  }
};

template <size_t N>
struct Traits<fixed_string<N>> {
    static unsigned hash(const fixed_string<N>& s) { return djb2(s.c_str(), s.size()); }
    static void dump(const fixed_string<N>& s) { kprintf("%s", s.c_str()); }
};

template <typename T, size_t N>
struct Traits<array<T, N>> {
    static unsigned hash(const array<T, N>& arr) {
        unsigned h = 5381;
        for (const auto& item : arr) {
            unsigned item_hash = Traits<T>::hash(item);
            h = ((h << 5) + h) + item_hash;
        }
        return h;
    }
    static void dump(const array<T, N>& arr) {
        kprintf("[");
        for (size_t i = 0; i < arr.size(); ++i) {
            Traits<T>::dump(arr[i]);
            if (i < arr.size() - 1) {
                kprintf(", ");
            }
        }
        kprintf("]");
    }
};

template <typename T>
struct Traits<span<T>> {
    static unsigned hash(const span<T>& s) {
        unsigned h = 5381;
        for (const auto& item : s) {
            unsigned item_hash = Traits<T>::hash(item);
            h = ((h << 5) + h) + item_hash;
        }
        return h;
    }
    static void dump(const span<T>& s) {
        kprintf("span([");
        for (size_t i = 0; i < s.size(); ++i) {
            Traits<T>::dump(s[i]);
            if (i < s.size() - 1) {
                kprintf(", ");
            }
        }
        kprintf("])");
    }
};

template <typename T, size_t N>
struct Traits<static_vector<T, N>> {
    static unsigned hash(const static_vector<T, N>& vec) {
        unsigned h = 5381;
        for (const auto& item : vec) {
            unsigned item_hash = Traits<T>::hash(item);
            h = ((h << 5) + h) + item_hash;
        }
        return h;
    }
    static void dump(const static_vector<T, N>& vec) {
        kprintf("static_vector([");
        for (size_t i = 0; i < vec.size(); ++i) {
            Traits<T>::dump(vec[i]);
            if (i < vec.size() - 1) {
                kprintf(", ");
            }
        }
        kprintf("])");
    }
};

template <typename T1, typename T2>
struct Traits<Pair<T1, T2>> {
    static unsigned hash(const Pair<T1, T2>& p) {
        unsigned h1 = Traits<T1>::hash(p.first);
        unsigned h2 = Traits<T2>::hash(p.second);
        return ((h1 << 5) + h1) + h2;
    }
    static void dump(const Pair<T1, T2>& p) {
        kprintf("(");
        Traits<T1>::dump(p.first);
        kprintf(", ");
        Traits<T2>::dump(p.second);
        kprintf(")");
    }
};

template <typename T, size_t MaxBits>
struct Traits<Bitmap<T, MaxBits>> {
    static unsigned hash(const Bitmap<T, MaxBits>& bm) {
        unsigned h = djb2(&bm.size(), sizeof(size_t));
        h = ((h << 5) + h) + djb2(bm.data(), bm.sizeInBytes());
        return h;
    }
    static void dump(const Bitmap<T, MaxBits>& bm) {
        kprintf("Bitmap(size=%zu, bits=[", bm.size());
        // Dump first few bits or a summary
        for (size_t i = 0; i < bm.size() && i < 64; ++i) { // Dump up to 64 bits
            kprintf("%d", bm.get(i) ? 1 : 0);
        }
        if (bm.size() > 64) {
            kprintf("...");
        }
        kprintf("])");
    }
};

template <typename Type, size_t MAX_SIZE>
struct Traits<Stack<Type, MAX_SIZE>> {
    static unsigned hash(const Stack<Type, MAX_SIZE>& s) {
        unsigned h = djb2(&s.size(), sizeof(size_t));
        // Hashing stack elements is tricky without exposing internal array or iterator
        // For simplicity, we'll just hash the size for now.
        // A more robust hash would iterate through elements if possible.
        return h;
    }
    static void dump(const Stack<Type, MAX_SIZE>& s) {
        kprintf("Stack(size=%zu, capacity=%zu)", s.size(), s.capacity());
        // Dumping elements would require a peek_at(index) or similar, not currently available.
        // If needed, this would be an enhancement to the Stack class itself.
    }
};

template <typename T>
struct Traits<optional<T>> {
    static unsigned hash(const optional<T>& opt) {
        if (opt.has_value()) {
            return Traits<T>::hash(opt.value());
        }
        return 0; // Consistent hash for empty optional
    }
    static void dump(const optional<T>& opt) {
        if (opt.has_value()) {
            kprintf("optional(");
            Traits<T>::dump(opt.value());
            kprintf(")");
        } else {
            kprintf("optional(empty)");
        }
    }
};

template <typename T>
struct Traits<OwnPtr<T>> {
    static unsigned hash(const OwnPtr<T>& ptr) {
        return djb2(&ptr.ptr(), sizeof(T*));
    }
    static void dump(const OwnPtr<T>& ptr) {
        kprintf("OwnPtr(0x%p", ptr.ptr());
        if (ptr.ptr()) {
            kprintf(", value=");
            Traits<T>::dump(*ptr.ptr());
        }
        kprintf(")");
    }
};

template <typename T>
struct Traits<RetainPtr<T>> {
    static unsigned hash(const RetainPtr<T>& ptr) {
        return djb2(&ptr.get(), sizeof(T*));
    }
    static void dump(const RetainPtr<T>& ptr) {
        kprintf("RetainPtr(0x%p", ptr.get());
        if (ptr.get()) {
            kprintf(", value=");
            Traits<T>::dump(*ptr.get());
        }
        kprintf(")");
    }
};

template <typename T>
struct Traits<rb_node<T>> {
    static unsigned hash(const rb_node<T>& node) {
        return Traits<T>::hash(node.value());
    }
    static void dump(const rb_node<T>& node) {
        kprintf("rb_node(value=");
        Traits<T>::dump(node.value());
        kprintf(", color=%s)", node.is_red() ? "red" : "black");
    }
};

template <typename T, unsigned MAX_NODES>
struct Traits<rb_tree<T, MAX_NODES>> {
    static unsigned hash(const rb_tree<T, MAX_NODES>& tree) {
        unsigned h = djb2(&tree.size(), sizeof(size_t));
        if (tree.root()) {
            h = ((h << 5) + h) + Traits<T>::hash(tree.root()->value());
        }
        return h;
    }
    static void dump(const rb_tree<T, MAX_NODES>& tree) {
        kprintf("rb_tree(size=%zu", tree.size());
        if (tree.root()) {
            kprintf(", root_value=");
            Traits<T>::dump(tree.root()->value());
        }
        kprintf(")");
    }
};
