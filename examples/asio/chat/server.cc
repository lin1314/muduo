#include "codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpServer.h>

#include <boost/bind.hpp>

#include <set>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include "lua_bind.h"
// #include "lua_tinker.h"
using namespace muduo;
using namespace muduo::net;

class ChatServer : boost::noncopyable
{
 public:
  ChatServer(EventLoop* loop,
             const InetAddress& listenAddr)
  : server_(loop, listenAddr, "ChatServer"),
    codec_(boost::bind(&ChatServer::onMessageQueue, this, _1))
  {
    server_.setConnectionCallback(
        boost::bind(&ChatServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
  }

  void start()
  {
    server_.start();
    LOG_INFO << "Server start success";
  }

  bool hasMessge(){ return messageQueue_.empty(); }
  void distillMessageQueue(std::list<MessageQueuePtr>& messageQueue) {
    // todo: 
    {
      MutexLockGuard lock(mutex_);
      messageQueue.swap(messageQueue_);
    }
  }

  // when timer event occur, create and push_front message to queue
  void onTimerMessage(const MessageQueuePtr& message) {
    {
      MutexLockGuard lock(mutex_);
      messageQueue_.push_front(message);
    }
  }
  
  void addMessage(const MessageQueuePtr& message) {
    {
      MutexLockGuard lock(mutex_);
      messageQueue_.push_back(message);
    }
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
      connections_.insert(conn);
    }
    else
    {
      connections_.erase(conn);
    }
  }

  void onMessageQueue(const MessageQueuePtr& message) {
    // todo: push to every conn's queue
    {
      MutexLockGuard lock(mutex_);
      messageQueue_.push_back(message);
    }
  }
  void onStringMessage(const TcpConnectionPtr&,
                       const string& message,
                       Timestamp)
  {
    for (ConnectionList::iterator it = connections_.begin();
        it != connections_.end();
        ++it)
    {
      codec_.send(get_pointer(*it), message);
    }
  }

  typedef std::set<TcpConnectionPtr> ConnectionList;
  TcpServer server_;
  LengthHeaderCodec codec_;
  ConnectionList connections_;
  
  mutable MutexLock mutex_;
  std::list<MessageQueuePtr> messageQueue_;// 通过clear 会不会存在泄露
};

ChatServer *g_server = 0;
bool bHandleMsg = true;
typedef std::list<MessageQueuePtr>::iterator MessageQueueIter; 
static inline void hanleMessageQueue() {
  while (bHandleMsg) {
    if (g_server && g_server->hasMessge()) {
      std::list<MessageQueuePtr> messageQueue;

      g_server->distillMessageQueue(messageQueue);
      for (MessageQueueIter itr = messageQueue.begin(); itr != messageQueue.end(); ++itr) {
        printf("[%s][%p][%s]\n", (*itr)->timestamp_.toString().c_str(), (*itr)->conn_, 
               (*itr)->message_.c_str());
      }
      messageQueue.clear();
    }
  }
}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);

    Thread threadHandleMessage(hanleMessageQueue, "hanleMessageQueue");
    threadHandleMessage.start();

    ChatServer server(&loop, serverAddr);
    g_server = &server;
    // server.start();

    loop.loop();
    threadHandleMessage.join();
  }
  else
  {
    printf("Usage: %s port\n", argv[0]);
  }
}

