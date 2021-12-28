#ifndef CHAT_SERVER_REDIS_REDIS_H_
#define CHAT_SERVER_REDIS_REDIS_H_
#include <thread>
#include <functional>
#include <hiredis/hiredis.h>
using std::string;
using std::function;
class Redis {
 public:
  Redis();
  ~Redis();
  bool LoadConfigFile();
  // 连接redis服务器
  bool Connect();
  // 向redis指定的通道channel发布消息
  bool Publish(int channel, string message);

  // 向redis指定的通道subscribe订阅消息
  // redis:SUBSCRIBE指令会造成线程阻塞等待通道里面发生消息
  // Subscribe函数只负责发送指令，决不接收通道消息，否则会和notifyMsg线程抢占响应资源
  // 故Subscribe函数将拆解原先会阻塞的SUBSCRIBE指令，
  // 函数使用redisAppendCommand + redisBufferWrite而不是直接redisCommand
  bool Subscribe(int channel);
  // 向redis指定的通道unsubscribe取消订阅消息
  bool Unsubscribe(int channel);
  // 初始化向业务层server上报通道消息的回调对象
  void InitNotifyHandler(function<void(int, string)> fn);
 
 private:
  string ip_;
  unsigned short port_ = 0; 
  // hiredis同步上下文对象，负责publish消息
  redisContext* publish_context_;
  // hiredis同步上下文对象，负责subscribe消息
  redisContext* subscribe_context_;
  // 回调操作，收到订阅的消息，给service层上报
  function<void(int, string)> notify_message_handler_;
  // 在独立线程中接收订阅通道中的消息
  void ObserverChannelMessage();
};
#endif