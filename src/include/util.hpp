#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <cassert>
#include <functional>

namespace Util {

  template <typename T, typename StrictWeakOrdering>
  inline void comparison_swap(T &lhs, T &rhs, StrictWeakOrdering cmp) {
    using std::swap;
    if (cmp(rhs, lhs)) {
      swap(lhs, rhs);
    }
  }

  template <typename T>
  inline void sort_asc(T &lhs, T &rhs) {
    comparison_swap(lhs, rhs, std::less<T>());
  }

  template <typename T>
  inline void sort_desc(T &lhs, T &rhs) {
    comparison_swap(lhs, rhs, std::greater<T>());
  }

  inline int bitcount(u64 bits) {
    // Only for 64-bit pltforms for now
    assert(sizeof(u64) == sizeof(unsigned long));
    return __builtin_popcountl((unsigned long)bits);
  }

  inline int hibit(u64 bits) {
    // Only for 64-bit pltforms for now
    assert(sizeof(u64) == sizeof(unsigned long));
    assert(bits != 0);

    return 63 - __builtin_clzl((unsigned long)bits);
  }

  inline u64 removebit(u64 bits, int bit) {
    return bits & ~((u64)1 << bit);
  }
  
} // namespace Util

#endif //ndef UTIL_HPP
