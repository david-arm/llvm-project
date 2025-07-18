//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <flat_set>

// template<class Key, class Compare, class KeyContainer, class Predicate>
//   typename flat_set<Key, Compare, KeyContainer>::size_type
//   erase_if(flat_set<Key, Compare, KeyContainer>& c, Predicate pred);

#include <deque>
#include <flat_set>
#include <functional>
#include <initializer_list>
#include <vector>

#include "test_macros.h"
#include "test_allocator.h"
#include "min_allocator.h"

// Verify that `flat_set` (like `set`) does NOT support std::erase.
//
template <class S>
concept HasStdErase = requires(S& s, typename S::value_type x) { std::erase(s, x); };
static_assert(HasStdErase<std::vector<int>>);
static_assert(!HasStdErase<std::flat_set<int>>);

template <class M>
constexpr M make(std::initializer_list<int> vals) {
  M ret;
  for (int v : vals)
    ret.emplace(v);
  return ret;
}

template <class M, class Pred>
constexpr void
test0(std::initializer_list<int> vals, Pred p, std::initializer_list<int> expected, std::size_t expected_erased_count) {
  M s = make<M>(vals);
  ASSERT_SAME_TYPE(typename M::size_type, decltype(std::erase_if(s, p)));
  assert(expected_erased_count == std::erase_if(s, p));
  assert(s == make<M>(expected));
}

struct NotBool {
  bool b;
  constexpr explicit operator bool() const { return b; }
};

template <class S>
constexpr void test_one() {
  // Test all the plausible signatures for this predicate.
  auto is1        = [](typename S::const_reference v) { return v == 1; };
  auto is2        = [](typename S::value_type v) { return v == 2; };
  auto is3        = [](const typename S::value_type& v) { return v == 3; };
  auto is4        = [](auto v) { return v == 4; };
  auto True       = [](const auto&) { return true; };
  auto False      = [](auto&&) { return false; };
  auto nonBoolIs1 = [](const auto& v) { return NotBool{v == 1}; };

  test0<S>({}, is1, {}, 0);

  test0<S>({1}, is1, {}, 1);
  test0<S>({1}, is2, {1}, 0);

  test0<S>({1, 2}, is1, {2}, 1);
  test0<S>({1, 2}, is2, {1}, 1);
  test0<S>({1, 2}, is3, {1, 2}, 0);

  test0<S>({1, 2, 3}, is1, {2, 3}, 1);
  test0<S>({1, 2, 3}, is2, {1, 3}, 1);
  test0<S>({1, 2, 3}, is3, {1, 2}, 1);
  test0<S>({1, 2, 3}, is4, {1, 2, 3}, 0);

  test0<S>({1, 2, 3}, True, {}, 3);
  test0<S>({1, 2, 3}, False, {1, 2, 3}, 0);

  test0<S>({1, 2, 3}, nonBoolIs1, {2, 3}, 1);
}

constexpr bool test() {
  test_one<std::flat_set<int>>();
  test_one<std::flat_set<int, std::less<int>, std::vector<int, min_allocator<int>>>>();
  test_one<std::flat_set<int, std::greater<int>, std::vector<int, test_allocator<int>>>>();
#ifndef __cpp_lib_constexpr_deque
  if (!TEST_IS_CONSTANT_EVALUATED)
#endif
  {
    test_one<std::flat_set<int, std::less<int>, std::deque<int, min_allocator<int>>>>();
    test_one<std::flat_set<int, std::greater<int>, std::deque<int, test_allocator<int>>>>();
  }
  test_one<std::flat_set<long>>();
  test_one<std::flat_set<double>>();

  return true;
}

int main(int, char**) {
  test();
#if TEST_STD_VER >= 26
  static_assert(test());
#endif

  return 0;
}
