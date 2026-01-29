#pragma once

#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace ge {
template <typename T, std::size_t Capacity> class ArrayVec {
public:
  using value_type = T;
  using size_type = std::size_t;
  using iterator = T *;
  using const_iterator = const T *;

  ArrayVec() noexcept : m_size(0) {}

  ~ArrayVec() { clear(); }

  ArrayVec(const ArrayVec &other) : m_size(0) {
    for (const auto &v : other) {
      push_back(v);
    }
  }

  ArrayVec &operator=(const ArrayVec &other) {
    if (this != &other) {
      clear();
      for (const auto &v : other) {
        push_back(v);
      }
    }
    return *this;
  }

  ArrayVec(ArrayVec &&other) noexcept : m_size(0) {
    for (auto &v : other) {
      push_back(std::move(v));
    }
    other.clear();
  }

  ArrayVec &operator=(ArrayVec &&other) noexcept {
    if (this != &other) {
      clear();
      for (auto &v : other) {
        push_back(std::move(v));
      }
      other.clear();
    }
    return *this;
  }

  // --------------------------------------------------
  // Capacity
  // --------------------------------------------------

  constexpr size_type capacity() const noexcept { return Capacity; }

  size_type size() const noexcept { return m_size; }

  bool empty() const noexcept { return m_size == 0; }

  bool full() const noexcept { return m_size == Capacity; }

  // --------------------------------------------------
  // Element access
  // --------------------------------------------------

  T &operator[](size_type i) {
    assert(i < m_size);
    return data()[i];
  }

  const T &operator[](size_type i) const {
    assert(i < m_size);
    return data()[i];
  }

  T &back() {
    assert(m_size > 0);
    return (*this)[m_size - 1];
  }

  const T &back() const {
    assert(m_size > 0);
    return (*this)[m_size - 1];
  }

  // --------------------------------------------------
  // Modifiers
  // --------------------------------------------------

  template <typename... Args> T &emplace_back(Args &&...args) {
    assert(m_size < Capacity);
    T *ptr = new (&storage()[m_size]) T(std::forward<Args>(args)...);
    ++m_size;
    return *ptr;
  }

  void push_back(const T &value) { emplace_back(value); }

  void push_back(T &&value) { emplace_back(std::move(value)); }

  void pop_back() {
    assert(m_size > 0);
    data()[m_size - 1].~T();
    --m_size;
  }

  void clear() noexcept {
    while (m_size > 0) {
      pop_back();
    }
  }

  // --------------------------------------------------
  // Iterators
  // --------------------------------------------------

  iterator begin() noexcept { return data(); }

  iterator end() noexcept { return data() + m_size; }

  const_iterator begin() const noexcept { return data(); }

  const_iterator end() const noexcept { return data() + m_size; }

  const_iterator cbegin() const noexcept { return begin(); }

  const_iterator cend() const noexcept { return end(); }

  // --------------------------------------------------
  // Raw access
  // --------------------------------------------------

  T *data() noexcept { return reinterpret_cast<T *>(storage()); }

  const T *data() const noexcept {
    return reinterpret_cast<const T *>(storage());
  }

  // erase_swap: removes element at index i by swapping with the last element
  void erase_swap(size_type i) {
    assert(i < m_size);
    if (i != m_size - 1) {
      std::swap((*this)[i], back());
    }
    pop_back();
  }

private:
  using Storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

  Storage *storage() noexcept { return m_storage; }

  const Storage *storage() const noexcept { return m_storage; }

  Storage m_storage[Capacity];
  size_type m_size;
};
} // namespace ge
