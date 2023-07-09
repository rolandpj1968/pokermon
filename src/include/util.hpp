#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
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

} // namespace Util

#endif //ndef UTIL_HPP
