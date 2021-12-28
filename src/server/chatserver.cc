#include "chatserver.h"

#include <functional>

#include "json.hpp"

#include "chatservice.h"
using namespace muduo;
using namespace muduo::net;
using namespace chat::server;
using namespace chat::service;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr,
                       const std::string& nameArg)
    : server_(loop, listenAddr, nameArg), loop_(loop) {
  // 注册链接处理回调
  // server_.setConnectionCallback(
  //   std::bind(&ChatServer::OnConnection, this, _1));
  // 使用c++11lambda表达式代替std::bind
  server_.setConnectionCallback(
      [this](const TcpConnectionPtr& conn) { this->OnConnection(conn); });

  // 注册信息处理回调
  // server_.setMessageCallback(
  //   std::bind(&ChatServer::OnMessage, this, _1, _2, _3));
  // 使用c++11lambda表达式代替std::bind
  server_.setMessageCallback(
      [this](const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
        this->OnMessage(conn, buffer, time);
      });

  // 设置线程数量
  server_.setThreadNum(2);
}

void ChatServer::Start() { server_.start(); }

void ChatServer::OnConnection(const TcpConnectionPtr& conn) {
  //
  if (!conn->connected()) {
    ChatService::Instance()->ClientExceptionClose(conn);
    conn->shutdown();
  }
}

void ChatServer::OnMessage(const TcpConnectionPtr& conn, Buffer* buffer,
                           Timestamp time) {
  // 数据反序列化
  std::string buf = buffer->retrieveAllAsString();
  json js = json::parse(buf);

  // 根据消息id类型决定/取出使用对应的处理函数
  auto msg_handler = ChatService::Instance()->Handler(js["msg_id"].get<int>());
  // 将消息的所有有关数据封装进这个functor：msg_handler的参数列中,
  // OnMessage函数最终会回调，把有关参数全部传进来，
  // 传给具体使用的对应的处理函数并调用之。
  msg_handler(conn, js, time);
  //
  // 因此实现了网络模块与业务处理模块的解耦合：
  // 不论业务处理模块那边需要这样增删改处理方式，这里的网络模块已经不需要改动了
  // 这里只需要根据id取出对应的处理方式并调用就行了
}
