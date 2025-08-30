# `_cxx`

This library implements a polyfill for some parts of the various C++ standard
libraries.

Please be extremely careful with the code in this library. Follow the rules:

- Do not include any headers from our main codebase. If something is needed,
  do not hesitate to copy-paste it.
- Implement only what is needed.
  There is no requirement of a complete feature set. For example, then a new
  missing container is being implemented, there is no need to implement all
  the methods. Also, if performance is not a concern, stick to the simplest
  possible implementation.
- Behavior must be consistent with the standard.
- Avoid polluting the `std` namespace.
- Be aware of differences between `#include` and `#include_next`.
  `#include_next` can be used to include a standard library header with the
  same name as the current header.

Each header in this library should stick to the following structure:

```cpp
// Assume that the header is named `feature`.
#pragma once
#pragma GCC system_header

// If the header is present in the standard library and we need to adjust it,
// then include the standard library header with the same name first.
//
// If we are implementing a feature missing in all the standard library
// implementations, then include some basic header (cstdlib, for example) so we
// still have access to macroes used to identify the current standard library
// implementation.
#include_next <feature>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef _LIBCPP_VERSION
_LIBCPP_BEGIN_NAMESPACE_STD

// Implement the libc++-specific parts of the feature.

_LIBCPP_END_NAMESPACE_STD
#endif // ifdef _LIBCPP_VERSION

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef __GLIBCXX__
namespace std _GLIBCXX_VISIBILITY(default) {
_GLIBCXX_BEGIN_NAMESPACE_VERSION

// Implement the libstdc++-specific parts of the feature.

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std _GLIBCXX_VISIBILITY(default)
#endif // ifdef __GLIBCXX__

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef _LIBCPP_VERSION
_LIBCPP_BEGIN_NAMESPACE_STD
#elifdef __GLIBCXX__
namespace std _GLIBCXX_VISIBILITY(default) {
  _GLIBCXX_BEGIN_NAMESPACE_VERSION
#endif

// Implement common parts of the feature.

#ifdef _LIBCPP_VERSION
_LIBCPP_END_NAMESPACE_STD
#elifdef __GLIBCXX__
  _GLIBCXX_END_NAMESPACE_VERSION
} // namespace std _GLIBCXX_VISIBILITY(default)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

```
