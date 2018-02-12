#ifndef _CONNECTION_MANAGER_
#define _CONNECTION_MANAGER_
#include <muduo/base/Types.h>
// #include <muduo/net/MessageQueue.h>
#include <muduo/net/TcpConnection.h>

namespace muduo {
namespace net {

// 消息结构, 存放连接消息队列
class ConnectionContext {
public:
  ConnectionContext(std::string & name, void * param);
  uint32_t handle() { return handle_; }
  MessageQueuePtr& messageQ() { return queue_; }


private:
  uint32_t handle_;
  TcpConnectionPtr conn_;
  MessageQueuePtr queue_; // todo: use weak_ptr
};
typedef std::shared_ptr<ConnectionContext> ConnectionContextPtr;

} // net
} // muduo
#endif // _CONNECTION_MANAGER_
