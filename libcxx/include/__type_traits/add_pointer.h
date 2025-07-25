//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_ADD_POINTER_H
#define _LIBCPP___TYPE_TRAITS_ADD_POINTER_H

#include <__config>
#include <__type_traits/is_referenceable.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_reference.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if !defined(_LIBCPP_WORKAROUND_OBJCXX_COMPILER_INTRINSICS)

template <class _Tp>
struct _LIBCPP_NO_SPECIALIZATIONS add_pointer {
  using type _LIBCPP_NODEBUG = __add_pointer(_Tp);
};

#  ifdef _LIBCPP_COMPILER_GCC
template <class _Tp>
using __add_pointer_t _LIBCPP_NODEBUG = typename add_pointer<_Tp>::type;
#  else
template <class _Tp>
using __add_pointer_t _LIBCPP_NODEBUG = __add_pointer(_Tp);
#  endif

#else
template <class _Tp, bool = __is_referenceable_v<_Tp> || is_void<_Tp>::value>
struct __add_pointer_impl {
  using type _LIBCPP_NODEBUG = __libcpp_remove_reference_t<_Tp>*;
};
template <class _Tp>
struct __add_pointer_impl<_Tp, false> {
  using type _LIBCPP_NODEBUG = _Tp;
};

template <class _Tp>
using __add_pointer_t = typename __add_pointer_impl<_Tp>::type;

template <class _Tp>
struct _LIBCPP_NO_SPECIALIZATIONS add_pointer {
  using type _LIBCPP_NODEBUG = __add_pointer_t<_Tp>;
};

#endif // !defined(_LIBCPP_WORKAROUND_OBJCXX_COMPILER_INTRINSICS)

#if _LIBCPP_STD_VER >= 14
template <class _Tp>
using add_pointer_t = __add_pointer_t<_Tp>;
#endif

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_ADD_POINTER_H
