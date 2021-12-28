#include "chatservice.h"
#include "public.h"

#include <string>
#include <functional>
#include <vector>
#include <muduo/base/Logging.h>

using chat::service::ChatService;
using muduo::net::TcpConnectionPtr;
using muduo::Timestamp;
using std::string;
using std::vector;
// 获取单实例对象
ChatService* ChatService::Instance() {
  static ChatService service;
  return &service;
}

// 注册消息id以及对应的Handler回调操作
ChatService::ChatService() {
  // msg_handler_map_存储不同的消息id以及对应的处理回调函数
  // 在此处注册消息id以及对应的处理回调函数
  // LOGIN_MSG消息id对应的是登录处理回调函数
  // 以下使用c++11lambda表达式代替std::bind 
  // using std::placeholders::_1;
  // using std::placeholders::_2;
  // using std::placeholders::_3;
  // msg_handler_map_.insert({LOGIN_MSG,
  //                          std::bind(&ChatService::Login, this, _1, _2,
  //                          _3)});
  msg_handler_map_.insert(
      {LOGIN_MSG, [this](const TcpConnectionPtr& conn, json& js,
                         Timestamp time) { this->Login(conn, js, time); }});
  // REG_MSG消息id对应的是注册处理回调函数
  msg_handler_map_.insert(
      {REG_MSG, [this](const TcpConnectionPtr& conn, json& js,
                       Timestamp time) { this->Register(conn, js, time); }});

  // TO_ONE_CHAT_MSG消息id对应的是一对一聊天业务
  msg_handler_map_.insert(
      {TO_ONE_CHAT_MSG,
       [this](const TcpConnectionPtr& conn, json& js, Timestamp time) {
         this->ToOneChat(conn, js, time);
       }});

  // 
  msg_handler_map_.insert({ADD_FRIEND_REQ_MSG, [this](const TcpConnectionPtr& conn,
                                                  json& js, Timestamp time) {
                             this->AddFriendRequest(conn, js, time);
                           }});
  
  // 查询好友申请列表                         
  msg_handler_map_.insert(
      {ADD_FRIEND_REQ_LIST_MSG,
       [this](const TcpConnectionPtr& conn, json& js, Timestamp time) {
         this->AddFriendRequestList(conn, js, time);
       }});
  // 添加好友
  msg_handler_map_.insert({ADD_FRIEND_MSG, [this](const TcpConnectionPtr& conn,
                                                  json& js, Timestamp time) {
                             this->AddFriend(conn, js, time);
                           }});

  // 删除好友
  msg_handler_map_.insert({DEL_FRIEND_MSG, [this](const TcpConnectionPtr& conn,
                                                  json& js, Timestamp time) {
                             this->DelFriend(conn, js, time);
                           }});
  // 创建群组
  msg_handler_map_.insert({CREATE_GROUP_MSG, [this](const TcpConnectionPtr& conn,
                                                  json& js, Timestamp time) {
                             this->CreateGroup(conn, js, time);
                           }});

  // 加入群组
  msg_handler_map_.insert(
      {JOIN_IN_GROUP_MSG,
       [this](const TcpConnectionPtr& conn, json& js, Timestamp time) {
         this->JoinInGroup(conn, js, time);
       }});

  // 群组聊天
  msg_handler_map_.insert({GROUP_CHAT_MSG, [this](const TcpConnectionPtr& conn,
                                                  json& js, Timestamp time) {
                             this->GroupChat(conn, js, time);
                           }});

  msg_handler_map_.insert({LEAVE_GROUP_MSG, [this](const TcpConnectionPtr& conn,
                                                  json& js, Timestamp time) {
                             this->LeaveGroup(conn, js, time);
                           }});                           
  // 注销退出登录
  msg_handler_map_.insert({LOGINOUT_MSG, [this](const TcpConnectionPtr& conn,
                                                json& js, Timestamp time) {
                             this->Loginout(conn, js, time);
                           }});
  //                         
  msg_handler_map_.insert(
      {REFRESHLISTS_MSG,
       [this](const TcpConnectionPtr& conn, json& js, Timestamp time) {
         this->RefreshLists(conn, js, time);
       }});
  // 在此处继续注册以后需要的消息id以及对应的处理回调函数

  //Redis Connection:
  if (redis_.Connect()) {
    // 注册通道消息到达的处理回调
    redis_.InitNotifyHandler([this](int channel, string msg) {
      this->HandlerRedisSubscribeMsg(channel, msg);
    });
  }
}

void ChatService::Login(const TcpConnectionPtr& conn, json& js,
                        Timestamp time) {
  LOG_INFO << "登录业务... ";
  int id = js["id"].get<int>();
  string pwd = js["password"];
  User user = user_model_.Query(id);
  if (user.id() == id && user.password() == pwd) {
    if (user.state() == "online") {
      json response;
      response["errno"] = 2;
      response["msg_id"] = LOGIN_MSG_ACK;
      response["err_msg"] = "该用户已登录，请重新输入账号";
      conn->send(response.dump());
    } else {
      // 登录成功
      // 更新状态信息:在线
      user.set_state("online");
      user_model_.UpdateState(user);
      { // critical section: 
        // 记录用户连接信息,加锁保证thread-safe
        std::lock_guard<std::mutex> lock(conn_mutex_);
        user_online_conn_map_.insert({id, conn});
      } // end of critical section

      // 客户端届时将以下有关字段保存/缓存在本地，减少服务器链接查询数据库的负荷
      json response;
      response["errno"] = 0;
      response["msg_id"] = LOGIN_MSG_ACK;
      response["id"] = user.id();
      response["user_name"] = user.user_name();
      // 查询当前在线状态下是否有离线消息
      vector<string> msg_vec = offline_msg_model_.Query(id);
      if (!msg_vec.empty()) {
        response["offline_msg"] = msg_vec;
        // Don't forget to remove offline message
        offline_msg_model_.Remove(id);
      }
      // 显示好友列表
      vector<User> friend_uvec = friend_model_.Query(id);
      if (!friend_uvec.empty()) {
        vector<string> friend_svec;
        for (const User& frid : friend_uvec) {
          json js;
          js["friend_id"] = frid.id();
          js["friend_name"] = frid.user_name();
          js["state"] = frid.state();
          friend_svec.push_back(js.dump());
          // 上线通知
          OnlineBroadast(frid.id(), user);
        }
        response["friends"] = friend_svec;
      }
      // 显示用户所在群组以及群员列表
      vector<Group> groups_and_users_vec = group_model_.QueryGroupsAndUsers(id);
      if (!groups_and_users_vec.empty()) {
        vector<string> groups_vec;
        for (const Group& group : groups_and_users_vec) {
          json group_js;
          group_js["id"] = group.group_id();
          group_js["group_name"] = group.group_name();
          group_js["desc"] = group.desc();
          vector<string> group_users_vec;
          for (const GroupUser& group_user : group.group_users()) {
            json js;
            js["id"] = group_user.id();
            js["user_name"] = group_user.user_name();
            js["state"] = group_user.state();
            js["role"] = group_user.role();
            group_users_vec.push_back(js.dump());
          }
          group_js["users"] = group_users_vec;
          groups_vec.push_back(group_js.dump());
        }
        response["groups"] = groups_vec;
      }
      conn->send(response.dump());
      // 以用户ID号为通道号，订阅redis消息队列通道
      redis_.Subscribe(id);
    }
  } else {
    json response;
    response["errno"] = 1;
    response["msg_id"] = LOGIN_MSG_ACK;
    response["err_msg"] = "用户名或密码错误!";
    conn->send(response.dump());
  }
}

void ChatService::OnlineBroadast(int friend_id, const User& from_user) {
  json broadcast;
  broadcast["msg_id"] = ONLINE_BROADCAST_MSG_ACK;
  broadcast["from_id"] = from_user.id();
  broadcast["from_username"] = from_user.user_name();  
  { // critical section
    std::lock_guard<std::mutex> lock(conn_mutex_);
    auto it = user_online_conn_map_.find(friend_id);
    if (it != user_online_conn_map_.end()) {
      it->second->send(broadcast.dump());
      return;
    }
  } // end of critical section
  User user = user_model_.Query(friend_id);
  if (user.state() == "online") {
    redis_.Publish(friend_id, broadcast.dump());
  }
}

void ChatService::Register(const TcpConnectionPtr& conn, json& js,
                           Timestamp time) {
  LOG_INFO << "注册业务... ";
  string user_name = js["user_name"];
  string pwd = js["password"];

  User user;
  user.set_user_name(user_name);
  user.set_password(pwd);
  bool is_reg_success = user_model_.Insert(&user);
  // 注册成功
  if (is_reg_success) {
    json response;
    response["errno"] = 0;
    response["msg_id"] = REG_MSG_ACK;
    response["id"] = user.id();
    response["reg_name"] = user_name;
    conn->send(response.dump());
  // 注册失败
  } else {
    json response;
    response["errno"] = 1;
    response["msg_id"] = REG_MSG_ACK;
    conn->send(response.dump());
  }
}

void ChatService::AddFriend(const TcpConnectionPtr& conn, json& js,
                            Timestamp time) {
  LOG_INFO << "好友通过业务... ";
  vector<int> sender_id_vec = js["add_friend_resp_yes_ids"];
  int user_id = js["user_id"].get<int>();
  vector<string> new_friend_vec;
  for (const int sender_id : sender_id_vec) {
    friend_model_.Insert(user_id, sender_id);
    add_friend_req_model.Remove(sender_id, user_id);
    User new_friend = user_model_.Query(sender_id);
    json new_js;
    new_js["new_friend_id"] = new_friend.id();
    new_js["new_friend_name"] = new_friend.user_name();
    new_js["new_friend_state"] = new_friend.state();
    new_friend_vec.push_back(new_js.dump());
    json to_sender;
    to_sender["msg_id"] = ADD_FRIEND_RESP_MSG_ACK;
    to_sender["from_id"] = js["user_id"];
    to_sender["from_name"] = js["user_name"];
    { 
      std::lock_guard<std::mutex> lk(conn_mutex_);
      auto it = user_online_conn_map_.find(new_friend.id());
      if (it != user_online_conn_map_.end()) {
        it->second->send(to_sender.dump());
      } else {
        User user = user_model_.Query(new_friend.id());
        if (user.state() == "online") {
          redis_.Publish(new_friend.id(), to_sender.dump());
        }
      }
    }
  }
  json response;
  response["errno"] = 0;
  response["msg_id"] = ADD_FRIEND_MSG_ACK;
  response["new_friends"] = new_friend_vec;
  conn->send(response.dump());
}

void ChatService::AddFriendRequest(const TcpConnectionPtr& conn, json& js,
                                   Timestamp time) {
  LOG_INFO << "好友申请业务... ";                                    
  int sender_id = js["sender_id"].get<int>();
  int to_id = js["to_id"].get<int>();
  if (user_model_.BoolQuery(to_id)) {
    add_friend_req_model.Insert(sender_id, to_id);
    
    {  // critical section
      std::lock_guard<std::mutex> lock(conn_mutex_);
      auto it = user_online_conn_map_.find(to_id);
      if (it != user_online_conn_map_.end()) {
        it->second->send(js.dump());
        return;
      }
    }  // end of critical section
    User user = user_model_.Query(to_id);
    if (user.state() == "online") {
      redis_.Publish(to_id, js.dump());
    } else {
      // 好友不在线，将离线消息存储在数据库中
      offline_msg_model_.Insert(to_id, js.dump());
    }
  }
}

void ChatService::AddFriendRequestList(const TcpConnectionPtr& conn, json& js,
                                       Timestamp time) {
  LOG_INFO << "请求好友申请列表... ";                                         
  json resp_js;
  json response;
  vector<std::pair<int, string>> add_friend_req_vec =
      add_friend_req_model.Query(js["user_id"].get<int>());
  response["msg_id"] = ADD_FRIEND_REQ_LIST_MSG_ACK;
  if (!add_friend_req_vec.empty()) {
    vector<string> add_friend_req_strvec;
    response["errno"] = 0;
    for (const auto& sender : add_friend_req_vec) {
      resp_js["sender_id"] = sender.first;
      resp_js["sender_name"] = sender.second;
      add_friend_req_strvec.push_back(resp_js.dump());
    }
    response["add_friend_req_list"] = add_friend_req_strvec;
    
  } 
  conn->send(response.dump());
}


void ChatService::DelFriend(const TcpConnectionPtr& conn, json& js,
                            Timestamp time) {
  LOG_INFO << "移除好友业务... ";                              
  int user_id = js["id"].get<int>();
  int friend_id = js["friend_id"].get<int>();
  if (user_model_.BoolQuery(friend_id)) {
    friend_model_.Delete(user_id, friend_id);
    json response;
    response["errno"] = 0;
    response["msg_id"] = DEL_FRIEND_MSG_ACK;
    response["friend_id"] = friend_id;
    conn->send(response.dump());
  } else {
    json response;
    response["errno"] = 1;
    response["msg_id"] = DEL_FRIEND_MSG_ACK;
    conn->send(response.dump());
  }
}

void ChatService::ToOneChat(const TcpConnectionPtr& conn, json& js,
                            Timestamp time) {
  LOG_INFO << "一对一聊天业务... ";                              
  int to_id = js["to_id"].get<int>();

  { // critical section
    std::lock_guard<std::mutex> lock(conn_mutex_);
    auto it = user_online_conn_map_.find(to_id);
    // 若好友在线，并且在同一台服务器，直接发送消息 
    if(it != user_online_conn_map_.end()) {
      it->second->send(js.dump());
      return;
    }
  } // end of critical section
  
  // 好友在线，但不在当前服务器，redis发布消息
  User user = user_model_.Query(to_id);
  if (user.state() == "online") {
    redis_.Publish(to_id, js.dump());
  } else {
    // 好友不在线，将离线消息存储在数据库中
    offline_msg_model_.Insert(to_id, js.dump());
  }
  
}

void ChatService::CreateGroup(const TcpConnectionPtr& conn, json& js,
                              Timestamp time) {
  LOG_INFO << "创建群组业务... ";                              
  int user_id = js["creator_id"].get<int>();
  string group_name = js["group_name"];
  string group_desc = js["group_desc"];
  string role = "creator";
  Group group(-1, group_name, group_desc);
  if (group_model_.CreateGroup(&group) == true) {
    group_model_.JoinInGroup(user_id, group.group_id(), role);
    json response;
    response["errno"] = 0;
    response["msg_id"] = CREATE_GROUP_MSG_ACK;
    response["group_id"] = group.group_id();
    conn->send(response.dump());
  } else {
    json response;
    response["errno"] = 1;
    response["msg_id"] = CREATE_GROUP_MSG_ACK;
    conn->send(response.dump());
  }
}

void ChatService::JoinInGroup(const TcpConnectionPtr& conn, json& js,
                              Timestamp time) {
  LOG_INFO << "加入群组业务... ";                              
  int user_id = js["user_id"].get<int>();
  int group_id = js["group_id"].get<int>();
  string role = "normal";
  if (group_model_.QueryGroupExisting(group_id)) {
    group_model_.JoinInGroup(user_id, group_id, role);
    json response;
    response["errno"] = 0;
    response["msg_id"] = JOIN_IN_GROUP_MSG_ACK;
    Group group = group_model_.QueryOneGroupAndMembers(user_id, group_id);
    int group_id = group.group_id();
    if (group_id != -1) {
      response["group_id"] = group_id;
      response["group_name"] = group.group_name();
      response["group_desc"] = group.desc();
      vector<GroupUser> members = group.group_users();
      vector<string> members_str;
      for (const GroupUser& member : members) {
        json js;
        js["id"] = member.id();
        js["user_name"] = member.user_name();
        js["state"] = member.state();
        js["role"] = member.role();
        members_str.push_back(js.dump());
      }
      response["group_members"] = members_str;
    }
    conn->send(response.dump());
  } else {
    json response;
    response["errno"] = 1;
    response["msg_id"] = JOIN_IN_GROUP_MSG_ACK;
    conn->send(response.dump());
  }
}

void ChatService::GroupChat(const TcpConnectionPtr& conn, json& js,
                            Timestamp time) {
  LOG_INFO << "群聊业务... ";                            
  int user_id = js["user_id"].get<int>();
  int group_id = js["group_id"].get<int>();
  vector<int> users_id_vec = group_model_.QueryGroupOtherUersId(user_id, group_id);
  { // critical section
    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (int id : users_id_vec) {
      auto it = user_online_conn_map_.find(id);
      if (it != user_online_conn_map_.end()) {
        it->second->send(js.dump());
      } else {
        // 在线，但不在当前服务器，redis发布消息
        User user = user_model_.Query(id);
        if (user.state() == "online") {
          redis_.Publish(id, js.dump());
        } else {
          // 好友不在线，将离线消息存储在数据库中
          offline_msg_model_.Insert(id, js.dump());
        }
      }
    }
  } // end of critical section
}

void ChatService::LeaveGroup(const TcpConnectionPtr& conn, json& js,
                            Timestamp time) {
  LOG_INFO << "退出群组业务... ";                              
  int user_id = js["id"].get<int>();
  int group_id = js["group_id"].get<int>();
  if (group_model_.QueryGroupExisting(group_id)) {
    group_model_.DeleteMember(user_id, group_id);
    json response;
    response["errno"] = 0;
    response["msg_id"] = LEAVE_GROUP_MSG_ACK;
    response["group_id"] = group_id;
    conn->send(response.dump());
  } else {
    json response;
    response["errno"] = 1;
    response["msg_id"] = LEAVE_GROUP_MSG_ACK;
    conn->send(response.dump());
  }
}

MsgHandler ChatService::Handler(int msg_id) { 
  auto it = msg_handler_map_.find(msg_id);
  if(it == msg_handler_map_.end()) { 
    // 若消息id没有对应的事件处理回调，报告错误日志
    // 返回一个默认的handler，除报告外什么都不做
    return [msg_id](const TcpConnectionPtr&, json&, Timestamp) {
      LOG_ERROR << "msg_id: " << msg_id << " can not find handler! ";
    };
  } else {
    return msg_handler_map_[msg_id];
  }
}

void ChatService::HandlerRedisSubscribeMsg(int user_id, string msg) {
  // 从另一台
  { // critical section
    std::lock_guard<std::mutex> lock(conn_mutex_);
    auto it = user_online_conn_map_.find(user_id);
    if (it != user_online_conn_map_.end()) {
      it->second->send(msg);
      return;
    }
  } // end of critical section
  offline_msg_model_.Insert(user_id, msg);
}

void ChatService::Loginout(const TcpConnectionPtr& conn, json& js,
                           Timestamp time) {
  int user_id = js["id"].get<int>();
  { // critical section 
    std::lock_guard<std::mutex> lock(conn_mutex_);
    auto it = user_online_conn_map_.find(user_id);
    if (it != user_online_conn_map_.end()) {
      user_online_conn_map_.erase(it);
    }
  } // end of section  
  
  // 取消redis消息通道   
  redis_.Unsubscribe(user_id);

  User loginout_user(user_id, "", "","offline");
  user_model_.UpdateState(loginout_user);
}

void ChatService::RefreshLists(const TcpConnectionPtr& conn, json& js,
                               Timestamp time) {
  LOG_INFO << "刷新业务... ";                                 
  json response;
  response["msg_id"] = REFRESHLISTS_MSG_ACK;
  int user_id = js["user_id"].get<int>();
  vector<User> friend_uvec = friend_model_.Query(user_id);
  if (!friend_uvec.empty()) {
    vector<string> friend_svec;
    for (const User& frid : friend_uvec) {
      json js;
      js["friend_id"] = frid.id();
      js["friend_name"] = frid.user_name();
      js["state"] = frid.state();
      friend_svec.push_back(js.dump());
    }
    response["friends"] = friend_svec;
  }
  // 显示用户所在群组以及群员列表
  vector<Group> groups_and_users_vec =
      group_model_.QueryGroupsAndUsers(user_id);
  if (!groups_and_users_vec.empty()) {
    vector<string> groups_vec;
    for (const Group& group : groups_and_users_vec) {
      json group_js;
      group_js["id"] = group.group_id();
      group_js["group_name"] = group.group_name();
      group_js["desc"] = group.desc();
      vector<string> group_users_vec;
      for (const GroupUser& group_user : group.group_users()) {
        json js;
        js["id"] = group_user.id();
        js["user_name"] = group_user.user_name();
        js["state"] = group_user.state();
        js["role"] = group_user.role();
        group_users_vec.push_back(js.dump());
      }
      group_js["users"] = group_users_vec;
      groups_vec.push_back(group_js.dump());
    }
    response["groups"] = groups_vec;
  }
  conn->send(response.dump());
}

void ChatService::ClientExceptionClose(const TcpConnectionPtr& conn) {
  User offline_user;
  {  // critical section
    std::lock_guard<std::mutex> lock(conn_mutex_);
    for (const auto& elem : user_online_conn_map_) {
      if (elem.second == conn) {
        int id = elem.first;
        offline_user.set_id(id);
        user_online_conn_map_.erase(id);
        break;
      }
    }
  }  // end of critical section
  // 取消redis消息通道
  redis_.Unsubscribe(offline_user.id());

  if (offline_user.id() != -1) {
    offline_user.set_state("offline");
    user_model_.UpdateState(offline_user);
  }
}

void ChatService::Reset() {
  user_model_.ResetAllState();
}