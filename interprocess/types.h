//  Copyright 2013, bitdewy@gmail.com
//  Distributed under the Boost Software License, Version 1.0.
//  You may obtain a copy of the License at
//
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef INTERPROCESS_TYPES_H_
#define INTERPROCESS_TYPES_H_

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
  explicit ScopeGuard(std::function<void()> onExitScope)
    : onExitScope_(onExitScope), dismissed_(false) {}

  ~ScopeGuard() {
    if (!dismissed_) {
      onExitScope_();
    }
  }

  void Dismiss() {
    dismissed_ = true;
  }

 private:
  std::function<void()> onExitScope_;
  bool dismissed_;
};

#define _LINENAME_CAT(name, line) name##line
#define _LINENAME(name, line) _LINENAME_CAT(name, line)

#define ON_SCOPE_EXIT(callback) ScopeGuard _LINENAME(EXIT, __LINE__)(callback)

class Connection;

typedef std::shared_ptr<Connection> ConnectionPtr;

typedef std::function<void(const ConnectionPtr&)> CloseCallback;

typedef
std::function<void(const ConnectionPtr&, const std::string&)> MessageCallback;

static const int kTimeout = 5000;

static const int kBufferSize = 4096;

}  // namespace interprocess

#endif  // INTERPROCESS_TYPES_H_
