#include "redis.h"
#include <iostream>
using std::cerr;
using std::endl;
using std::cout;
bool Redis::LoadConfigFile() {
  FILE* pf = fopen("../conf/redis.ini","r");
  if (pf == nullptr) {
    cout << "配置文件不存在" << endl;
    return false;
  }
  while(!feof(pf)) {
    char line[64] = {0};
    fgets(line, 64, pf);
    string str = line;
    int idx = str.find("=", 0);
    if (idx == -1) {
      continue;
    }
    int end_idx = str.find('\n', idx);
    string key = str.substr(0, idx);
    string value = str.substr(idx + 1, end_idx - idx -1);
    if (key == "ip") {
      ip_ = value;
    } else if (key == "port") {
      port_ = atoi(value.c_str());
    }
  }
  return true;
}

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr) {
  if (!LoadConfigFile()) {
    return;
  }
}

Redis::~Redis() {
  if (publish_context_ != nullptr) {
    redisFree(publish_context_);
  }
  if (subscribe_context_ != nullptr) {
    redisFree(subscribe_context_);
  } 
}

bool Redis::Connect() {
  publish_context_ = redisConnect(ip_.c_str(),port_);
  if (publish_context_ == nullptr) {
    cerr << "链接Redis失败" << endl;
    return false;
  }
  subscribe_context_ = redisConnect(ip_.c_str(),port_);
  if (subscribe_context_ == nullptr) {
    cerr << "链接Redis失败" << endl;
    return false;
  }
  std::thread t([&]() { ObserverChannelMessage(); });
  t.detach();
  cout << "链接Redis服务成功" << endl;
  return true;
}

bool Redis::Publish(int channel, string message) {
  redisReply* reply = static_cast<redisReply*>(redisCommand(
      publish_context_, "PUBLISH %d %s", channel, message.c_str()));
  if (reply == nullptr) {
    cerr << "发布消息失败" << endl;
    return false;
  }
  freeReplyObject(reply);
  return true;
}

bool Redis::Subscribe(int channel) {
  if (redisAppendCommand(this->subscribe_context_, "SUBSCRIBE %d",
                         channel) == REDIS_ERR) {
    cerr << "订阅消息失败" << endl;
    return false;
  }
  // redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
  int done = 0;
  while (!done) {
    if (redisBufferWrite(this->subscribe_context_, &done) == REDIS_ERR) {
      cerr << "订阅消息失败" << endl;
      return false;
    }
  }
  return true;
}

bool Redis::Unsubscribe(int channel) {
  if (redisAppendCommand(this->subscribe_context_, "UNSUBSCRIBE %d", channel) ==
      REDIS_ERR) {
    cerr << "取消订阅消息失败" << endl;
    return false;
  }
  int done = 0;
  while (!done) {
    if (redisBufferWrite(this->subscribe_context_, &done) == REDIS_ERR) {
      cerr << "取消订阅消息失败" << endl;
      return false;
    }
  }
  return true;
}

void Redis::ObserverChannelMessage(){
  redisReply* reply = nullptr;
  // redisGetReply会阻塞
  while (redisGetReply(this->subscribe_context_, (void**)&reply) == REDIS_OK) {
    if (reply != nullptr &&
        reply->element[2] != nullptr && reply->element[2]->str != nullptr) {
      notify_message_handler_(atoi(reply->element[1]->str),
                              reply->element[2]->str);
    }
    freeReplyObject(reply);
  }
  cerr << "观察线程(等待通道消息)退出!" << endl;
}

void Redis::InitNotifyHandler(std::function<void(int, string)> fn) {
  this->notify_message_handler_ = fn;
}
