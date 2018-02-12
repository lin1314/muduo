/*************************************************************************
  > File Name: Handle.cpp
  > Author: ljy
  > Mail: 2818880298@qq.com
  > Created Time: Fri 09 Feb 2018 08:13:30 PM CST
 ************************************************************************/
#include <muduo/base/Handle.h>
#include <muduo/net/MessageQueue.h>
#include <muduo/net/ConnectionMgr.h>

namespace muduo 
{
namespace net
{

uint32_t Handle::regHandle(const ConnectionContextPtr& ctx) {
  MutexLockGuard lock(mutexHaldle_);
  while (true) {
    // 高8位留给harbor跨节点预留, 这里类似取模
    uint32_t handle = (handleIndex_++) & HANDLE_MASK;
    if (handle2Context_[handle]) {
      continue;
    }
    // handle2Context_[handle] = ctx;
    handle2Context_.insert(std::make_pair(handle, ctx));

    handle |= harbor_; // 高 8bit 加上harbor
    return handle;
  }
}

bool
Handle::nameHandle(uint32_t handle, const std::string& name) {
  MutexLockGuard lock(mutexName_);
  if (name2Handle_[name]) {
    return false;
  }
  handle = handle & HANDLE_MASK;

  name2Handle_[name] = handle;
  handle2Name_[handle] = name;
  return true;
}

// 反查context
ConnectionContextPtr Handle::grab(uint32_t handle) const {
  MutexLockGuard lock(mutexHaldle_);

  uint32_t rawHandle = handle & HANDLE_MASK; // clean high 8bit
  HandleContextMap::const_iterator got = handle2Context_.find(rawHandle);
  if (got == handle2Context_.end()) {
    return nullptr;
  }
  ConnectionContextPtr ctx(got->second);
  if (ctx && ctx->handle() == handle ) {
    return ctx;
  }
  return nullptr; // ctx handle not match 
}

// 移除hangdle
bool 
Handle::retire(uint32_t handle) {
  bool ret = false;
  uint32_t rawHandle = handle & HANDLE_MASK; // clean high 8bit
  {
    MutexLockGuard lock(mutexHaldle_);
    HandleContextMap::const_iterator got = handle2Context_.find(rawHandle);
    if (got == handle2Context_.end()) {
      ret = false;
    }
    ConnectionContextPtr ctx(got->second);
    if (ctx && ctx->handle() == handle ) {
      handle2Context_.erase(rawHandle);
      ret = true;
    }
  }

  if (ret) {
    MutexLockGuard lock(mutexName_);
    std::string && name = std::move(handle2Name_[rawHandle]);
    name2Handle_.erase(name);
    handle2Name_.erase(rawHandle);
  }
  return ret; // ctx handle not match or not found
}

// 按名字查找
uint32_t 
Handle::findByName(const std::string& name) {
  MutexLockGuard lock(mutexName_);
  return name2Handle_[name];
}
#if 0
#endif

} // net
} // muduo
