#include "chatserver.h"
#include <iostream>
#include <signal.h>
#include <muduo/net/EventLoop.h>
#include "chatservice.h"
// 处理服务器程序中断后，重置用户的离线状态信息
void ResetHandler(int) {
  chat::service::ChatService::Instance()->Reset();
  exit(0);

}


int main(int argc, char** argv) {
  if (argc < 3) {
    printf("无效输入!输入例子:./ChatServer 127.0.0.1 6000");
    exit(-1); 
  }
  char* ip = argv[1];
  uint16_t port = atoi(argv[2]);
  signal(SIGINT, ResetHandler);

  muduo::net::EventLoop loop;
  muduo::net::InetAddress addr(ip, port);
  chat::server::ChatServer server(&loop, addr, "ChatServer");

  server.Start();
  loop.loop();

  return 0;
}