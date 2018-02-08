#ifndef MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
#define MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
// extern "C" {
#include <muduo/base/Types.h>
// #include <stdlib.h>
// }

typedef muduo::string Message;
// type is endcoding in MessageQueue.sz high 8bit
// # if __WORDSIZE == 64
// #  define SIZE_MAX		(18446744073709551615UL)
// # else
// #  define SIZE_MAX		(4294967295U)
// # endif
#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1)*8)
class MessageQueue : public boost::enable_shared_from_this<MessageQueue>
{
public:
  // todo:change void* data --> int8_t *data
  //
  MessageQueue(const muduo::net::TcpConnectionPtr& conn, const void* data, 
                  const size_t sz, const muduo::Timestamp timestamp) : 
    conn_(conn), sz_(sz), timestamp_(timestamp) {
    size_t len = sz_ & MESSAGE_TYPE_MASK;
    data_ = malloc(len*sizeof(char));
    memcpy(data_, data, len);
  }

  MessageQueue(const void* data, const size_t sz ) : sz_(sz) {
    size_t len = sz_ & MESSAGE_TYPE_MASK;
    data_ = malloc(len*sizeof(char));
    memcpy(data_, data, len);
  }
  ~MessageQueue() {
    free(data_);
    data_ = NULL;
  }
  // todo1: use vector<char> replace data_;
  // todo2: use muduo::Buffer to this situation
  muduo::net::TcpConnectionPtr conn_;
  void *data_;
  size_t sz_; // 高八位存储消息类型.
  muduo::Timestamp timestamp_;
};
typedef boost::shared_ptr<MessageQueue> MessageQueuePtr;


class LengthHeaderCodec : boost::noncopyable
{
 public:
  typedef boost::function<void (const muduo::net::TcpConnectionPtr&,
                                const muduo::string& message,
                                muduo::Timestamp)> StringMessageCallback;
  typedef boost::function<void (const MessageQueuePtr&)> QueueMessageCallback;

  explicit LengthHeaderCodec(const QueueMessageCallback& cb)
    : messageQueueCallback_(cb)
  {
  }

  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp receiveTime)
  {
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
      // FIXME: use Buffer::peekInt32()
      const int32_t len = buf->peekInt32();
      if (len > 65536 || len < 0)
      {
        LOG_ERROR << "Invalid length " << len;
        conn->shutdown();  // FIXME: disable reading
        break;
      }
      else if (buf->readableBytes() >= len + kHeaderLen)
      {
        buf->retrieve(kHeaderLen);
        // 按消息头里面的度, 读取数据, 并把它转为对应的结构
        MessageQueuePtr messageQueue(new MessageQueue(conn, 
                                                      buf->peek(), len, 
                                                      receiveTime));
        messageQueueCallback_(messageQueue);
        buf->retrieve(len);
      }
      else
      {
        break;
      }
    }
  }

  // FIXME: TcpConnectionPtr
  void send(muduo::net::TcpConnection* conn,
            const muduo::StringPiece& message)
  {
    muduo::net::Buffer buf;
    buf.append(message.data(), message.size());
    int32_t len = static_cast<int32_t>(message.size());
    int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
    buf.prepend(&be32, sizeof be32);
    conn->send(&buf);
  }

 private:
  QueueMessageCallback messageQueueCallback_;
  const static size_t kHeaderLen = sizeof(int32_t);
};

#endif  // MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
