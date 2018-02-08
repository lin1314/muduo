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
extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

#include "lua_tinker.h"
#include "ttylua.h"
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
        // printf("[%s][%p][%s]\n", (*itr)->timestamp_.toString().c_str(), (*itr)->conn_, 
               // (*itr)->message_.c_str());
      }
      messageQueue.clear();
    }
  }
}

void threadHandleTTY() {
    lua_State *L = lua_open();
    if (L == NULL) {
      l_message("server", "cannot create state: not enough memory");
      return ;
    }

    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    lua_gc(L, LUA_GCRESTART, 0);
    luaL_openlibs(L);
    lua_settop(L,0);//清空栈
    // todo: read chunk string
    size_t len;
    const char* strBuffer = NULL;
    while(true) {
      if (readStdin(L) != -1) {
        strBuffer = lua_tolstring(L, -1, &len);

        MessageQueuePtr messageQueue(new MessageQueue(strBuffer, len));
        // LOG_INFO << "strBuffer = " << strBuffer;
        g_server->addMessage(messageQueue);
        lua_settop(L, 0);
      }
    }
}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc >= 0)
  {
    EventLoop loop;
    // uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(5678);
    ChatServer server(&loop, serverAddr);
    g_server = &server;

    // 消息处理线程
    Thread threadHandleMessage(hanleMessageQueue, "hanleMessageQueue");
    threadHandleMessage.start();

    // 终端输入线程
    Thread threadTTY(threadHandleTTY, "threadHandleTTY");
    threadTTY.start();

    // 全局状态机
    lua_State *L = lua_open();
    if (L == NULL) {
      l_message(argv[0], "cannot create state: not enough memory");
      return EXIT_FAILURE;
    }

    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    lua_gc(L, LUA_GCRESTART, 0);
    luaL_openlibs(L);
    lua_settop(L,0);//清空栈

    char strArgvCmd[100] = {'\0'};
    for (int i=1; i < argc; ++i){
      strcat(strArgvCmd, argv[i]);
      strcat(strArgvCmd, " ");
    }
    lua_pushstring(L, strArgvCmd);
    int status = handle_luainit(L);// 加载初始AppMain.lua
    report(L, status);

    server.start();
    loop.loop();
    threadHandleMessage.join();

    lua_close(L);
  }
  else
  {
    printf("Usage: %s port\n", argv[0]);
  }
}

