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

#include <vector>
#include <queue>
#include <list>

namespace muduo
{
namespace net
{
// MSG type is encoding in MSG.sz high 8bit
#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1)*8)

  struct Msg {
    uint32_t sourceHandle_;
    int session_;
    std::vector<char> data_;
    size_t sz;
  }; 

  class MsgQueue{
  public:
    explicit MsgQueue(uint32_t handle);
    uint32_t handle() const { return handle_; }

  private:
    typedef std::unique_ptr<Msg> MsgUnqPtr;
    mutable MutexLock mutex_;
    uint32_t handle_;
    bool release_;
    bool inGlobal_;
    int overload_;
    int overloadThreasHold;
    std::list<MsgUnqPtr> queue_;
  }; 

  class GlobalMQ {
  public:
    typedef std::unique_ptr<MessageQueue> MsgQueueUnqPtr;

    explicit GlobalMQ();
    void push(MsgQueue *queue);
    MsgQueueUnqPtr pop();

  private:
    mutable MutexLock mutex_;
    std::list<MsgQueueUnqPtr> Q_; 
  };


#endif
