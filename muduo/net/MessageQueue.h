/*************************************************************************
  > File Name: MessageQueue.h
  > Author: ljy
  > Mail: 2818880298@qq.com
  > Created Time: Thu 08 Feb 2018 11:26:31 AM CST
 ************************************************************************/

#ifndef _MESSAGEQUEUE_H
#define _MESSAGEQUEUE_H
#include <muduo/CommonDef.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/base/Singleton.h>

#include <vector>
#include <queue>
#include <list>
#include <memory>

namespace muduo
{
namespace net
{
// Message type is encoding in Message.sz high 8bit
#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1)*8)

#define MQ_IN_GLOBAL 1
#define MQ_OVERLOAD 1024
#define MAX_GLOBAL_MQ 0x10000

class ConnectionContext;
typedef std::shared_ptr<ConnectionContext> ConnectionContextPtr;

class Message;
class MessageQueue;
class GlobalMQ;
typedef std::unique_ptr<Message> MessageUPtr;
typedef std::shared_ptr<MessageQueue> MessageQueuePtr;
#define GlobalMQ_PUSH GlobalMQ::getSingleton()::push
#define GlobalMQ_POP GlobalMQ::getSingleton()::pop

class Message
{
public:
  Message(uint32_t sourceHandle, int session, 
      const char* buf, size_t len, size_t sz) :
    sourceHandle_(sourceHandle),
    // session_(session),
    data_(buf, buf+len),
    sz_(sz)
  {}
private:
  uint32_t sourceHandle_{0};
  // 暂时还未支持
  // int session_{0};
  std::vector<char> data_;
  size_t sz_{0};
}; 

class GlobalMQ : public Singleton<GlobalMQ>
{
public:
  // 取出一条messageQ
  // 按优先级, 处理至少一条消息
  void addMessage(ConnectionContextPtr& ctx, const char* buf, size_t len, size_t sz);
  bool dispatchMessage(int weight);

private:
  // 添加消息, 若messageQ不在全局队列则,加入; 
  void addMessage(MessageQueuePtr & messageQ, MessageUPtr && message);
  // todo: handle message with lua
  void dispatchMessageImp(ConnectionContextPtr & ctx, MessageUPtr && message) {}
  MessageQueuePtr nextMessageQ() {
    return std::move(queue_.take());
  }

  void pushMessageQ(MessageQueuePtr & messageQ) {
    queue_.put(std::move(messageQ));
  }

private:
  // mutable MutexLock mutex_;
  MessageQueuePtr currMq_; // todo:使用unique_ptr 当前正在处理的队列
  BlockingQueue<MessageQueuePtr> queue_;
};

// 只在globalQ中存在shared_ptr
// 其他地方只需要弱引用
class MessageQueue 
{ // 同时只被一条线程访问
  // not thread_safe
public:
  explicit MessageQueue(uint32_t handle) : handle_(handle) { }
  uint32_t handle() const { return handle_; }
  int overload();

  size_t size() { return queue_.size(); }
  void push_back(MessageUPtr && message) {
    queue_.put(std::move(message));
  }

  // addMessage(uint32_t sourceHandle, int session, const char* buf, size_t len, size_t sz) {
    // MessageUPtr message(new Message(sourceHandle, session, buf, len, sz));
    // queue_.put(std::move(message));
  // }

  // 设置 overload_threshold (length翻倍)
  // 弹出后, 修改inGlobal标记为0, 以通知投递到globalQ
  MessageUPtr pop();

  void push(MessageUPtr&& message) { queue_.put(std::move(message)); }

  void setInGlobal(bool flag) { inGlobal_ = flag; }
  bool inGlobal() const { return inGlobal_; }

private:
  uint32_t handle_{0};
  bool inGlobal_{true};
  int overload_{0};
  int overloadThreshold_{MQ_OVERLOAD};
  BlockingQueue<MessageUPtr> queue_;
}; 

} // muduo
} // net

#endif
