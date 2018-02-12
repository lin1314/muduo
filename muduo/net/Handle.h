#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>
#include <muduo/base/Singleton.h>

#include <string>
#include <unordered_map>
#include <memory>

namespace muduo 
{
namespace base
{
#define HANDLE_MASK 0xFFFFFF
#define HANDLE_REMOTE_SHIFT 24

class muduo::net::ConnectionContext;

class Handle : Singleton<Handle>
{
public:
  typedef std::shared_ptr<muduo::net::ConnectionContext> ConnectionContextPtr;

  void setHabor_(uint32_t harbor){ // shift到高 8bit
    harbor_ = static_cast<uint32_t>((harbor & 0xff) << HANDLE_REMOTE_SHIFT); 
  }
  // 分配handle
  uint32_t regHandle(const ConnectionContextPtr & ctx);
  bool nameHandle(uint32_t handle, const std::string& name);

  // 移除hangdle
  bool retire(uint32_t handle);

  // 反查context
  ConnectionContextPtr grab(uint32_t handle) const;
  uint32_t findByName(const std::string& name);


private:
  typedef std::unordered_map<uint32_t, ConnectionContextPtr> HandleContextMap;

  uint32_t harbor_{0};
  uint32_t handleIndex_{1};

  mutable MutexLock mutexHaldle_;
  HandleContextMap handle2Context_;

  mutable MutexLock mutexName_; // 给服务命名, 存储两份name实体, 可优化
  std::unordered_map<std::string, uint32_t> name2Handle_;
  std::unordered_map<uint32_t, std::string> handle2Name_;

};

#define HANDLE_INST Handle::getSingleton()

} // base
} // muduo
