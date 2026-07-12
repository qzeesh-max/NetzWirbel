/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef OVERALIGNED_ALLOCATOR_HPP
#define OVERALIGNED_ALLOCATOR_HPP

#include <iostream>
#include <new>
#include <stddef.h>
#include <string>
#include <type_traits>
#include <limits>

template <typename T, size_t Alignment>
class OverAlignedAllocator {
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  template <typename U>
  struct rebind {
    using other = OverAlignedAllocator<U, Alignment>;
  };

  OverAlignedAllocator() = default;
  template <typename U>
  OverAlignedAllocator(const OverAlignedAllocator<U, Alignment> &) noexcept {}

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }

  pointer allocate(size_type n) {
    constexpr size_t IdealAlignment = Alignment > alignof(T)
    ? Alignment
    : alignof(T);

  #ifndef _MSC_VER
    void *ptr = aligned_alloc(IdealAlignment, n * sizeof(T));
  #else
    void *ptr = _aligned_malloc(n * sizeof(T), IdealAlignment);
  #endif

    if (!ptr) [[unlikely]] {
      throw std::bad_alloc();
    }

    return static_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) {
  #ifndef _MSC_VER
    free(p);
  #else
    _aligned_free(p);
  #endif
  }

  size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  template <typename U, typename... Args>
  void construct(U *p, Args &&... args) {
    ::new ((void *)p) U(std::forward<Args>(args)...);
  }

  template <typename U>
  void destroy(U *p) {
    p->~U();
  }

  bool operator==(const OverAlignedAllocator &) noexcept {
    return true;
  }
  bool operator!=(const OverAlignedAllocator &) noexcept {
    return false;
  }
};

#endif
