//  Copyright 2014, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_UNIQUE_HANDLE_H_
#define INTERPROCESS_UNIQUE_HANDLE_H_

namespace interprocess {

template <typename T, typename Traits>
class unique_handle {
  struct bool_struct { int member; };
  using bool_type = int bool_struct::*;
public:
  explicit unique_handle(T value = Traits::invalid())
    : value_(value) {}
  unique_handle(unique_handle&& other)
    : value_(other.release()) {}
  unique_handle& operator=(unique_handle&& other) {
    reset(other.release());
    return *this;
  }
  ~unique_handle() { close(); }
  T get() const { return value_; }
  bool reset(T value = Traits::invalid()) {
    if (value_ != value) {
      close();
      value_ = value;
    }
    return *this;
  }
  T release() {
    auto value = value_;
    value_ = Traits::invalid();
    return value;
  }
  operator bool_type() {
    return Traits::invalid() != value_ ? &bool_struct::member : nullptr;
  }
private:
  unique_handle(const unique_handle&);
  unique_handle& operator=(const unique_handle&);
  bool operator==(const unique_handle&);
  bool operator!=(const unique_handle&);
  void close() {
    if (*this) {
      Traits::close(value_);
    }
  }
  T value_;
};

}  // namespace interprocess

#endif