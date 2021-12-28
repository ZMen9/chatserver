// Copyright 2021
// License
// Author: ZeMeng Zheng
// This is ...
#ifndef CHAT_SERVER_CHATSERVER_H_
#define CHAT_SERVER_CHATSERVER_H_

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
// 不要使用using namespace muduo::net,会污染命名空间

namespace chat {

namespace server {

//
// 聊天服务器服务端
//
class ChatServer {
 public:
  ChatServer(muduo::net::EventLoop* loop, 
             const muduo::net::InetAddress& KListenAddr,
             const std::string& KNameArg);

  // 启动聊天服务
  void Start();

 private:
  // （定义）上报链接的回调函数
  void OnConnection(const muduo::net::TcpConnectionPtr&);

  // （定义）上报读写相关信息的回调函数
  void OnMessage(const muduo::net::TcpConnectionPtr&,
                muduo::net::Buffer*, 
                muduo::Timestamp);

      // 组合muduo库的Tcp server类对象，实现服务器功能
  muduo::net::TcpServer server_;

  // 组合muduo库的事件循环类对象指针
  muduo::net::EventLoop* loop_;
};

} // namespace server

} // namespace chat

#endif  // CHAT_SERVER_CHATSERVER_H_