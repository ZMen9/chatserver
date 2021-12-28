// Copyright 2021
// License
// Author: ZeMeng Zheng
// This is ...
#ifndef CHAT_SERVER_CHATSERVICE_H_
#define CHAT_SERVER_CHATSERVICE_H_



#include <mutex>
#include <unordered_map>
#include <utility>

#include <json.hpp>
#include <muduo/net/TcpConnection.h>
#include "friend_model.h"
#include "group_model.h"
#include "offline_message_model.h"
#include "user_model.h"
#include "add_friend_req_model.h"
#include  "redis.h"

using json = nlohmann::json;

using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr& conn,
                                      json& js, muduo::Timestamp)>;

namespace chat {

namespace service {
// 表示处理消息的事件回调类型
class ChatService {
 public:
  // 获取单实例对象
  static ChatService* Instance();

  ChatService(const ChatService&) = delete;
  // 处理登录业务
  void Login(const muduo::net::TcpConnectionPtr& conn, json& js,
             muduo::Timestamp time);
  //
  void OnlineBroadast(int friend_id,const User& from_user);

  // 处理注册业务
  void Register(const muduo::net::TcpConnectionPtr& conn, json& js,
                muduo::Timestamp time);
  // 添加好友业务
  void AddFriend(const muduo::net::TcpConnectionPtr& conn, json& js,
                 muduo::Timestamp time);
                 
  void AddFriendRequest(const muduo::net::TcpConnectionPtr& conn, json& js,
                        muduo::Timestamp time);

  void AddFriendRequestList(const muduo::net::TcpConnectionPtr& conn, json& js,
                            muduo::Timestamp time);
  

  // 删除好友业务
  void DelFriend(const muduo::net::TcpConnectionPtr& conn, json& js,
                 muduo::Timestamp time);

  // 一对一聊天业务
  void ToOneChat(const muduo::net::TcpConnectionPtr& conn, json& js,
                 muduo::Timestamp time);

  // 创建群聊
  void CreateGroup(const muduo::net::TcpConnectionPtr& conn, json& js,
                   muduo::Timestamp time);
  // 加入群聊
  void JoinInGroup(const muduo::net::TcpConnectionPtr& conn, json& js,
                   muduo::Timestamp time);

  // 群组聊天
  void GroupChat(const muduo::net::TcpConnectionPtr& conn, json& js,
                 muduo::Timestamp time);

  void LeaveGroup(const muduo::net::TcpConnectionPtr& conn, json& js,
                  muduo::Timestamp time);

  // 获取消息对应的处理器
  MsgHandler Handler(int msg_id);
  // redis回调处理器:跨服务器通信，从redis消息队列获取订阅消息并发送给目的方
  void HandlerRedisSubscribeMsg(int user_id, string msg);
  // 处理用户注销退出
  void Loginout(const muduo::net::TcpConnectionPtr& conn, json& js,
                muduo::Timestamp time);
  void RefreshLists(const muduo::net::TcpConnectionPtr& conn, json& js,
                    muduo::Timestamp time);
  // 处理客户端异常退出
  void ClientExceptionClose(const muduo::net::TcpConnectionPtr& conn);
  // 重置服务
  void Reset();

 private:
  // 私有构造函数以达成单例模式
  ChatService();
  // unordered_map的pair存储消息id以及对应的业务处理方法
  // 构造时已经添加好处理方法，不存在运行时动态添加处理器，故thread-safe
  std::unordered_map<int, MsgHandler> msg_handler_map_{};
  // 存储在线用户的通信连接,可以自定义哈希函数，否则对TcpConnectionPtr查询时只能O(n)
  // not thread-safe
  std::unordered_map<int, const muduo::net::TcpConnectionPtr&>
      user_online_conn_map_{};
  // 保证以上user_online_conn_map_线程安全的互斥锁
  std::mutex conn_mutex_;

  // 组合用户数据处理类
  UserModel user_model_;

  OfflineMsgModel offline_msg_model_;
  FriendModel friend_model_;
  GroupModel group_model_;
  AddFriendReqModel add_friend_req_model;
  // redis消息队列处理跨服务器通信
  Redis redis_;
};

}  // namespace service

}  // namespace chat

#endif  // CHAT_SERVER_CHATSERVICE_H_