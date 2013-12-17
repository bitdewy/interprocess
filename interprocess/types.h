//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_TYPES_H_
#define INTERPROCESS_TYPES_H_

#include <windows.h>
#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <string>

namespace interprocess {

// taken from boost.noncopyable
// http://www.boost.org/doc/libs/1_54_0/boost/noncopyable.hpp

namespace noncopyable_ {  // protection from unintended ADL

class noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:  // emphasize the following members are private
  noncopyable(const noncopyable&);
  noncopyable& operator=(const noncopyable&);
};

}  // namespace noncopyable_

typedef noncopyable_::noncopyable noncopyable;

class ScopeGuard : public noncopyable {
 public:
  explicit ScopeGuard(std::function<void()> on_exit_scope)
    : on_exit_scope_(on_exit_scope) {}

  ~ScopeGuard() {
    if (!dismissed_) {
      on_exit_scope_();
    }
  }

  void Dismiss() const {
    dismissed_ = true;
  }

 private:
  std::function<void()> on_exit_scope_;
  mutable std::atomic_bool dismissed_;
};

#define _LINENAME_CAT(name, line) name##line
#define _LINENAME(name, line) _LINENAME_CAT(name, line)

#define ON_SCOPE_EXIT(callback) ScopeGuard _LINENAME(EXIT, __LINE__)(callback)

class Connection;

typedef std::shared_ptr<Connection> ConnectionPtr;

typedef std::function<void(HANDLE, HANDLE, HANDLE)> NewConnectionCallback;

typedef std::function<void(const ConnectionPtr&)> CloseCallback;

typedef
std::function<void(const ConnectionPtr&, const std::string&)> MessageCallback;

class ConnectionExcepton : public std::exception {
 public:
  explicit ConnectionExcepton(const char* what_arg)
    : exception(what_arg) {}

  explicit ConnectionExcepton(const std::string& what_arg)
    : exception(what_arg.c_str()) {}

  virtual const char* what() const {
    return exception::what();
  }
};

typedef std::function<void(const std::exception_ptr&)> ExceptionCallback;

static const int kTimeout = 5000;

static const int kBufferSize = 4096;

inline void raise() {
  auto msg = std::string("ConnectionExcepton GetLastError = ");
  msg.append(std::to_string(GetLastError()));
  std::rethrow_exception(
    std::make_exception_ptr(ConnectionExcepton(msg)));
}

template<typename Predicate>
void raise_exception_if(Predicate pred) {
  if (pred()) {
    raise();
  }
}

template<typename Predicate, typename ScopedGuard>
void raise_exception_if(Predicate pred, const ScopedGuard& guard) {
  if (pred()) {
    guard.Dismiss();
    raise();
  }
}

inline void raise_exception() {
  raise_exception_if([]() { return true; });
}

template <typename Function>
inline void call_if_exist(Function f) {
  if (f) {
    f();
  }
}

template <typename Function, typename... Arg>
inline void call_if_exist(Function f, Arg... arg) {
  if (f) {
    f(std::forward<Arg>(arg)...);
  }
}

}  // namespace interprocess

#endif  // INTERPROCESS_TYPES_H_
