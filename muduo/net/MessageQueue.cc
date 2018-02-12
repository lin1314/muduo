/*************************************************************************
  > File Name: MessageQueue.cpp
  > Author: ljy
  > Mail: 2818880298@qq.com
  > Created Time: Thu 08 Feb 2018 11:24:52 AM CST
 ************************************************************************/
#include <muduo/net/MessageQueue.h>

#include <muduo/CommonDef.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Handle.h>
#include <muduo/base/Logging.h>
#include <muduo/net/ConnectionMgr.h>

#include <vector>
#include <assert.h>

namespace muduo
{
namespace net
{

void GlobalMQ::addMessage(ConnectionContextPtr& ctx, 
                            const char* buf, size_t len, size_t sz) {
  uint32_t handle = ctx->handle();
  MessageQueuePtr messageQ = ctx->messageQ();
  // 消息队列为空, 会被垃圾回收.
  // 这边如果有新消息, 需要再创建
  messageQ = messageQ? messageQ : std::make_shared<MessageQueue>(handle);
  MessageUPtr message(new Message(handle, 0, buf, len, sz));

  addMessage(messageQ, std::move(message));
}

//@weight 处理优先级, 可以控制一次处理一个message_queue中的几条消息
bool GlobalMQ::dispatchMessage(int weight) {
  MessageQueuePtr messageQ = (currMq_)? currMq_ : nextMessageQ();
  currMq_ = messageQ;

  if (!messageQ) { 
    return false; 
  }
  uint32_t handle = messageQ->handle();
  ConnectionContextPtr ctx = HANDLE_INST.grab(handle);
  // 异常情况, messageQ没能反查拥有者
  // 不再把messageQ放回, 释放这条messageQ
  if (!ctx) {
    return false; 
  }

  // 按 优先级, 循环处理消息
  MessageUPtr message;
  for (int i=0, n=1; i < n; ++i) {
    message = messageQ->pop();
    // 此消息队列已经清空
    if (!message) {
      return false; 
    } 
    else if (i == 0 && weight >=0 ) { 
      // 第一次循环的时候, 修改n
      n = messageQ->size();
      n >>= weight;
    }
    int overload = messageQ->overload();
    if (overload > 0) {
      LOG_ERROR << handle << " may overload, message queue length " << overload;
    }
    dispatchMessageImp(ctx, std::move(message));
  }

  // 处理完一条链接的消息, 检查处理下一条链接消息
  MessageQueuePtr nextQ = nextMessageQ();
  if (nextQ) {
    pushMessageQ(messageQ);
    currMq_ = nextQ;
  }
  return true;
}

void GlobalMQ::addMessage(MessageQueuePtr & messageQ, MessageUPtr && message) {
  messageQ->push_back(std::move(message));
  if (!messageQ->inGlobal()) {
    messageQ->setInGlobal(true);
    pushMessageQ(messageQ);
  }
}

MessageUPtr MessageQueue::pop() {
  int length = size();
  while (length > overloadThreshold_) {
    overload_ = length;
    overloadThreshold_ *= 2;
  }

  MessageUPtr message;
  // reset overloadThreshold when queue is empty
  if (length > 0) {
    message = queue_.take();
  } else {
    overloadThreshold_ = 0;
  }
  return message;
}

int MessageQueue::overload() {
  if (!overload_) {
    return 0;
  }
  int overload = overload_;
  overload_ = 0;
  return overload;
}


} // muduo
} // net
