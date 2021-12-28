#include <ctime>
#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <chrono>
#include <regex>
#include <atomic>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#include "json.hpp"
#include "user.h"
#include "group.h"
#include "public.h"
#include "global_data.h"

using json = nlohmann::json;
using std::vector;
using std::string;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::regex;
using std::map;


void LoginResponse(json&);
void RegisterResponse(const json& response_js);
void UpdateAllFriendsList(const json&, std::map<int, User>&);
void UpdateAllGroupsList(const json&, std::vector<Group>&);
void GetCurrentUserRelatedLists(const json&);
void ShowLocalFriendsList(int);
void ShowLocalGroupsAndMembersList(int);
void ShowCurrentUserData();
void ShowCurrentUserOfflineMsg(json&);


void ReadTaskHandler(int clientfd);

void Help(int fd = 0);

void OneChat(int);

void AddFriendRequest(int);
void FriendAddResponse(int);
void DeleteFriend(int);
void CreateGroup(int);
void CreateGroupResponse(const json&);

void JoinInGroup(int);

void GroupChat(int);
void LeaveGroup(int);
void LoginOut(int);
void RefreshRelatedLists(int clienfd);

void mainMenu(int clientfd);

string CurrentTime();

std::map<int, string> command_map = {
  {1, "help:帮助列表 格式:1或h"},
  {2, "chat:好友聊天 格式:2或chat"},
  {3, "addfriend:添加好友 格式:3或add"},
  {4, "addresponse_manage:好友申请列表 格式:4或resp"}, 
  {5, "deletefriend:移除好友 格式:5或del"},
  {6, "showfriendlist:显示好友列表 格式:6或flist"},
  {7, "groupchat:群组聊天 格式:7或gchat"},
  {8, "creategroup:创建群组 格式:8或create"},
  {9, "joingroup:加入群组 格式:9或join"},
  {10,"leavegroup:退出群组 格式:10或leave"},
  {11, "showGroupList:显示群组列表 格式:11或glist"},
  {12, "refresh:刷新列表 格式:12或r"},
  {13, "loginout:注销 格式:13或quit"},

};

std::unordered_map<string, std::function<void(int)>> command_hanlder_map = {
  {"1", Help},
  {"h", Help},
  {"2", OneChat},
  {"chat", OneChat},
  {"3", AddFriendRequest},
  {"add", AddFriendRequest},
  {"4", FriendAddResponse},
  {"resp", FriendAddResponse},
  {"5", DeleteFriend},
  {"del", DeleteFriend},
  {"6", ShowLocalFriendsList},
  {"flist", ShowLocalFriendsList},
  {"7", GroupChat},
  {"gchat", GroupChat},
  {"8", CreateGroup},
  {"create", CreateGroup},
  {"9", JoinInGroup},
  {"join", JoinInGroup},
  {"10", LeaveGroup},
  {"leave", LeaveGroup},
  {"11", ShowLocalGroupsAndMembersList},
  {"glist", ShowLocalGroupsAndMembersList},
  {"12", RefreshRelatedLists},
  {"r", RefreshRelatedLists},
  {"13", LoginOut},
  {"quit", LoginOut}
};

int main(int argc, char** argv) {
  if (argc < 3) {
    cerr << "无效输入!输入例子:./ChatClient 127.0.0.1 8000" << endl;
    cout << endl;
    exit(-1); 
  }
  
  char* ip = argv[1];
  uint16_t port = std::atoi(argv[2]);

  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clientfd == -1) {
    cerr << "socket创建失败" << endl;
    exit(-1);
  }
  
  sockaddr_in server;
  memset(&server, 0, sizeof(sockaddr_in));
  
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(ip);

  if (connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in))) {
    cerr << "连接服务器失败" << endl;
    close(clientfd);
    exit(-1);
  }

  //
  sem_init(&rwsem, 0, 0);
  // 启动接收线程负责接收/读服务端发来的数据
  std::thread readTask(ReadTaskHandler, clientfd);
  readTask.detach();

  for (;;) {
    cout << "==================================" << endl;
    cout << " 1.登录 " << endl;
    cout << " 2.注册 " << endl;
    cout << " 3.退出" << endl;
    cout << "==================================" << endl;
    cout << "请输入以上您需要的业务号码: ";
    string input;
    getline(cin, input);
    regex rgx("[1-9]");
    if (std::regex_match(input, rgx)) {
        int choice = atoi(input.c_str());
        switch (choice) {
          case 1: {
            int id = 0;
            char pwd[18] = {0};
            cout << "用户账号(id号): ";
            string input_id;
            getline(cin,input_id);
            regex rgx("[0-9]+");
            if (std::regex_match(input_id, rgx)) {
              int id = atoi(input_id.c_str());
              cout << "用户密码: ";
              cin.getline(pwd, 18);
              json js;
              js["msg_id"] = LOGIN_MSG;
              js["id"] = id;
              js["password"] = pwd;
              string request;
              try {
                request = js.dump();
              } catch (nlohmann::detail::exception& e) {
                if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
                break;
              }

              is_login_success = false;
              int len = send(clientfd, request.c_str(),
                             strlen(request.c_str()) + 1, 0);
              if (len == -1) {
                cerr << "向服务器发送登录消息失败..." << request << endl;
              } else {
                sem_wait(&rwsem);
                if (is_login_success == true) {
                  is_main_menu_running = true;
                  mainMenu(clientfd);
                }
              }
            } else {
              cerr << "您输入的账号ID格式不正确!" << endl;
            }
            break;
          }
          case 2: {
            char user_name[32] ={0};
            char pwd[32] = {0};
            cout << "用户昵称:";
            cin.getline(user_name, 32);
            cout << "用户密码:";
            cin.getline(pwd, 32);
            if (user_name[0] !='\0' && pwd[0] !='\0') {
              json js;
              js["msg_id"] = REG_MSG;
              js["user_name"] = user_name;
              js["password"] = pwd;
              string request;
              try {
                request = js.dump();
              } catch (nlohmann::detail::exception& e) {
                if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
                break;
              }
              int len = send(clientfd, request.c_str(),
                             strlen(request.c_str()) + 1, 0);
              if (len == -1) {
                cerr << "向服务器发送注册消息失败..." << request << endl;
              } else {
                sem_wait(&rwsem);
              }
            } else {
              cerr << "注册失败:用户名,密码为空" << endl;
            }
            break;
          }
          case 3: {
            sem_destroy(&rwsem);
            close(clientfd);
            exit(0);
          }
          default: {
            cerr << "无效输入!您应该输入1或2或3中的一个" << endl;
            break;
          }
        }
    } else {
      cerr << "无效输入!您应该输入1或2或3中的一个" << endl;
    } 
  } 
  return 0;
}

void LoginResponse(json& response_js) {
  if (response_js["errno"].get<int>() != 0) {
    cerr << response_js["err_msg"] << endl;
    is_login_success = false;
  } else {  // 登录信息无误，
            // 记录下所有有关信息并保存在程序中的两个记录块中
    current_user.set_id(response_js["id"].get<int>());
    current_user.set_user_name(response_js["user_name"]);
    GetCurrentUserRelatedLists(response_js);
    ShowCurrentUserData();
    ShowCurrentUserOfflineMsg(response_js);
    is_login_success = true;
  }
}

void RegisterResponse(const json& response_js) {
  if (response_js["errno"].get<int>() != 0) {
    cerr << "注册失败" << endl;
  } else {
    cout << "----------------------------------" << endl;
    cout << "注册成功!" << endl;
    cout << "用户" << response_js["reg_name"].get<string>()
         << ",请记住您的用户ID号, 登录时使用此ID号: "
         << response_js["id"].get<int>() << endl;
    cout << "建议您将此登录ID号保存到备忘录" << endl;
  }
}
void UpdateAllFriendsList(const json& response_js,
                       std::map<int, User>& current_user_friends_list) {
  current_user_friends_list.clear();
  if (response_js.contains("friends")) {
    vector<string> friends_vec = response_js["friends"];
    for (const string& the_friend : friends_vec) {
      json js = json::parse(the_friend);
      User my_frined;
      my_frined.set_id(js["friend_id"].get<int>());
      my_frined.set_user_name(js["friend_name"].get<string>());
      my_frined.set_state(js["state"].get<string>());
      current_user_friends_list.insert({my_frined.id(), my_frined});
    }
  }
}

void UpdateAllGroupsList(const json& response_js,
                      std::vector<Group>& current_user_groups_list) {
  current_user_groups_list.clear();
  groups_id_and_name.clear();
  if (response_js.contains("groups")) {
    vector<string> groups_vec = response_js["groups"];
    for (const string& group : groups_vec) {
      json group_js = json::parse(group);
      Group the_group;
      int group_id = group_js["id"].get<int>();
      the_group.set_group_id(group_id);
      string group_name = group_js["group_name"];
      the_group.set_group_name(group_name);
      the_group.set_desc(group_js["desc"].get<string>());
      groups_id_and_name.insert({group_id, group_name});

      vector<string> group_users_vec = group_js["users"];
      for (const string& group_user : group_users_vec) {
        json js = json::parse(group_user);
        GroupUser user;
        user.set_id(js["id"].get<int>());
        user.set_user_name(js["user_name"].get<string>());
        user.set_state(js["state"].get<string>());
        user.set_role(js["role"].get<string>());
        the_group.group_users().push_back(user);
      }
      current_user_groups_list.push_back(the_group);
    }
  }
}

void GetCurrentUserRelatedLists(const json& response_js) {
  // 记录下用户自己的好友信息并保存
  UpdateAllFriendsList(response_js, current_user_friends_list);

  // 记录下用户所在群组以及群员的有关信息并保存
  UpdateAllGroupsList(response_js, current_user_groups_list);
}

void ShowLocalFriendsList(int) {
  cout << "____________________您的好友____________________" << endl;
  if (!current_user_friends_list.empty()) {
    for (const auto& it : current_user_friends_list) {
      cout << "好友昵称:" << it.second.user_name() << endl;
      cout << "好友ID号:" << it.second.id() << " 状态:" << it.second.state()
           << endl;
      cout << "-----------------------------------------" << endl;
    }
  }
}

void ShowLocalGroupsAndMembersList(int) {
  cout << "____________________您的群组____________________" << endl;
  if (!current_user_groups_list.empty()) {
    for (const Group& the_group : current_user_groups_list) {
      cout << "群组ID:" << the_group.group_id() << endl;
      cout << "群组名称:" << the_group.group_name() << endl;
      cout << "群简介:" << the_group.desc() << endl;
      cout << "...有以下群成员:" << endl;
      for (const GroupUser& group_user : the_group.group_users()) {
        cout << "群成员昵称:" << group_user.user_name() << endl;
        cout << "ID号:" << group_user.id() << "-状态:" << group_user.state()
             << "-群权限:" << group_user.role() << endl;
        cout << "-----------------------------------------" << endl;
      }
    }
  }
}

void ShowCurrentUserData() {
  cout << "====================用户信息====================" << endl;
  cout << "您的登录ID:" << current_user.id() << endl;
  cout << "您的用户名:" << current_user.user_name() << endl;
  ShowLocalFriendsList(0);
  ShowLocalGroupsAndMembersList(0);
  cout << "================================================" << endl;
}

void ShowCurrentUserOfflineMsg(json& response_js) {
  if (response_js.contains("offline_msg")) {
    vector<string> msg_vec = response_js["offline_msg"];
    for (string& msg : msg_vec) {
      json js;
      try {
        js = json::parse(msg);
      } catch (json::parse_error e) {
        cout << "message: " << e.what() << '\n'
             << "exception id: " << e.id << '\n'
             << "byte position of error: " << e.byte <<endl;
        return;
      }
      cout << "您有离线消息:" << endl;
      int msg_type = js["msg_id"].get<int>();
      if (msg_type == TO_ONE_CHAT_MSG) {
        cout << "私信:" << js["time"].get<string>() << endl;
        cout << "[" << js["id"] << "] " << js["user_name"].get<string>()
             << " 说:" << js["msg"].get<string>() << endl;
      } else if (msg_type == GROUP_CHAT_MSG) {
        cout << "群消息:" << js["time"].get<string>() << endl;
        cout << "在"
             << "[" << js["group_id"] << "]" << js["group_name"].get<string>()
             << "中" << endl;
        cout << "[" << js["user_id"] << "] " << js["user_name"].get<string>()
             << " 说:" << js["msg"].get<string>() << endl;
      } else if (msg_type == ADD_FRIEND_REQ_MSG) {
        cout << "有人申请添加为您的好友:"
             << "[" << js["sender_id"] << "]" << js["sender_name"].get<string>()
             << endl;
        cout << "您可以到申请管理中进行处理" << endl;
      }
    }
  }
}

void ReadTaskHandler(int clientfd) {
  for (;;) {
    char buffer[4096] = {0};
    int len = recv(clientfd, buffer, 4096, 0);
    if (len == -1 || len == 0) {
      close(clientfd);
      exit(-1);
    }

    json js = json::parse(buffer);
    int msg_type = js["msg_id"].get<int>();
    if (msg_type == LOGIN_MSG_ACK) {
      LoginResponse(js);
      sem_post(&rwsem);
      continue;
    }
    
    if (msg_type == REG_MSG_ACK) {
      RegisterResponse(js);
      sem_post(&rwsem);
      continue;
    }
    
    if (msg_type == CREATE_GROUP_MSG_ACK) {
      cout << endl;
      CreateGroupResponse(js);
      sem_post(&rwsem);
      continue;
    }
    if (msg_type == ADD_FRIEND_REQ_LIST_MSG_ACK) {
      cout << endl;
      add_friend_req_list.clear();
      if (js.contains("add_friend_req_list")) {
        vector<string> list_strvec = js["add_friend_req_list"];
        if (!list_strvec.empty()) {
          std::lock_guard<std::mutex> lk(m);
          for (const string& add_req : list_strvec) {
            json js = json::parse(add_req);
            int sender_id = js["sender_id"].get<int>();
            string sender_name = js["sender_name"].get<string>();
            add_friend_req_list.push_back({sender_id, sender_name});
          }
        }
      }
      cv.notify_one();
      continue;
    }

    if (msg_type == ADD_FRIEND_REQ_MSG) {
      cout << endl;
      cout << "有人申请添加为您的好友:" << "[" << js["sender_id"] << "]" 
           << js["sender_name"] << endl;
      cout << "您可以到申请管理中进行处理" << endl;
      continue;
    }
    if (msg_type == ADD_FRIEND_MSG_ACK) {
      std::lock_guard<std::mutex> lk(m);
      vector<string> new_friends = js["new_friends"];
      for (const string& new_friend : new_friends) {
        json js = json::parse(new_friend);
        User my_new_friend;
        my_new_friend.set_id(js["new_friend_id"].get<int>());
        my_new_friend.set_user_name(js["new_friend_name"].get<string>());
        my_new_friend.set_state(js["new_friend_state"].get<string>());
        current_user_friends_list.insert({my_new_friend.id(), my_new_friend});
      }
      //cv.notify_one();
      continue;
    }
    if (msg_type == ADD_FRIEND_RESP_MSG_ACK) {
      std::lock_guard<std::mutex> lk(m);
      User new_friend(js["from_id"].get<int>(), js["from_name"].get<string>(),"", "online");
      current_user_friends_list.insert({new_friend.id(), new_friend});
      continue;
    }
    if (msg_type == DEL_FRIEND_MSG_ACK) {
      std::lock_guard<std::mutex> lk(m);
      if (js["errno"] == 0) {
        current_user_friends_list.erase(js["friend_id"].get<int>());
      }
    }
    
    if (msg_type == TO_ONE_CHAT_MSG) {
      cout << endl;
      cout << "私信:" << js["time"].get<string>() << endl;
      cout << "[" << js["id"] << "] " << js["user_name"].get<string>()
           << " 说:" << js["msg"].get<string>() << endl;
      continue;
    }
    if (msg_type == ONLINE_BROADCAST_MSG_ACK) {
      int friend_id = js["from_id"].get<int>();
      cout << endl;
      cout << "您的好友:"
           << "[" << friend_id << "]"
           << js["from_username"].get<string>() << "已上线" << endl;
      auto it = current_user_friends_list.find(friend_id);
      if (it != current_user_friends_list.end()) {
        it->second.set_state("online");
      }
      continue;
    }
    if (msg_type == GROUP_CHAT_MSG) {
      cout << endl;
      cout << "群消息:" << js["time"].get<string>() << endl;
      cout << "在" << "[" << js["group_id"].get<int>() << "]" 
           << js["group_name"].get<string>() << "中" << endl;
      cout << "[" << js["user_id"].get<int>() << "] "
           << js["user_name"].get<string>() << " 说:" << js["msg"].get<string>()
           << endl;
      continue;
    }
    if (msg_type == JOIN_IN_GROUP_MSG_ACK) {
      cout << endl;
      if (js["errno"] == 0) {
        if (js.contains("group_members")) {
          vector<string> group_members_str = js["group_members"];
          Group new_group;
          new_group.set_group_id(js["group_id"].get<int>());
          new_group.set_group_name(js["group_name"].get<string>());
          new_group.set_desc(js["group_desc"].get<string>());

          for (const string& member : group_members_str) {
            json js = json::parse(member);
            GroupUser user;
            user.set_id(js["id"].get<int>());
            user.set_user_name(js["user_name"].get<string>());
            user.set_state(js["state"].get<string>());
            user.set_role(js["role"].get<string>());
            new_group.group_users().push_back(user);
          }
          std::lock_guard<std::mutex> lk(m);
          current_user_groups_list.push_back(new_group);
          groups_id_and_name.insert(
              {js["group_id"].get<int>(), js["group_name"].get<string>()});
          cout << "加入该群成功" << endl;
        }
      } else {
        cout << "该群不存在" << endl;
      }

      cv.notify_one();
      continue;
    } 
    if (msg_type == LEAVE_GROUP_MSG_ACK) {
      int group_id = js["group_id"].get<int>();
      std::lock_guard<std::mutex> lk(m);
      if (js["errno"] == 0) {
        auto it = current_user_groups_list.begin();
        while (it != current_user_groups_list.end()) {
          if (it->group_id() == group_id) {
            current_user_groups_list.erase(it);
            groups_id_and_name.erase(group_id);
            break;
          } else {
            ++it;
          }
        }
      }
    }
    if (msg_type == REFRESHLISTS_MSG_ACK) {
      std::lock_guard<std::mutex> lk(m);
      UpdateAllFriendsList(js, current_user_friends_list);
      UpdateAllGroupsList(js, current_user_groups_list);
      cv.notify_one();
      continue;
    }
  }
}

void mainMenu(int clientfd) {
  Help();
  char buffer[1024];
  while(is_main_menu_running) {
    cout << "请输入指令: ";
    cin.getline(buffer,1024);
    if (strlen(buffer) == 0) {
      continue;
    }
    string command(buffer);
    auto it = command_hanlder_map.find(command);
    if (it == command_hanlder_map.end()) {
      cerr << "无效输入!" << endl;
      continue;
    }
    it->second(clientfd);
  }
}

void Help(int) {
  cout << "====================命令菜单====================" << endl;
  for (const auto &c : command_map) {
    cout << c.first << "." << c.second << endl;
  }
  cout << "================================================" << endl;
}

void OneChat(int clientfd) {
  cout << "好友聊天..." << endl;
  cout << "请输入想要聊天的好友ID:";
  bool is_continue = false;
  string input_id;
  int friend_id;
  getline(cin, input_id);
  regex rgx("[0-9]+");
  if (std::regex_match(input_id, rgx)) {
    friend_id = atoi(input_id.c_str());
    if (friend_id != current_user.id()) {
      auto it = current_user_friends_list.find(friend_id);
      if (it != current_user_friends_list.end()) {
        do {
          char message[256];
          cout << "发送内容:";
          cin.getline(message, 256);
          json js;
          js["msg_id"] = TO_ONE_CHAT_MSG;
          js["id"] = current_user.id();
          js["user_name"] = current_user.user_name();
          js["to_id"] = friend_id;
          js["msg"] = message;
          js["time"] = CurrentTime();
          string buffer;
          try {
            buffer = js.dump();
          } catch (nlohmann::detail::exception& e) {
            if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
            return;
          }
          int len =
              send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
          if (len == -1) {
            cerr << "发送消息失败 -> " << buffer << endl;
          } else {
            cout << "消息发送成功" << endl;
          }

          string input;
          for (int i = 0; i < 3; ++i) {
            cout << "继续发送? y/n: ";
            getline(cin, input);
            if (input == "yes" || input == "y") {
              is_continue = true;
              break;
            } else if (input == "no" || input == "n") {
              is_continue = false;
              break;
            }
          }
        } while (is_continue);
      } else {
        cout << "当前id用户非您的好友,发送失败" << endl;
      }
    } else {
      cout << "您输入的是自己的ID" << endl;
    }
  } else {
    cout << "您填入的ID号格式有误" << endl;
  }
}

void AddFriendRequest(int clientfd) {
  cout << "添加好友..." << endl;
  cout << "请输入您要添加的好友ID:";
  string input_id;
  getline(cin, input_id);
  regex rgx("[0-9]+");
  if (std::regex_match(input_id, rgx)) {
    int to_id = atoi(input_id.c_str());
    if (to_id != current_user.id()) {
      json js;
      js["msg_id"] = ADD_FRIEND_REQ_MSG;
      js["sender_id"] = current_user.id();
      js["sender_name"] = current_user.user_name();
      js["to_id"] = to_id;
      string buffer;
      try {
        buffer = js.dump();
      } catch (nlohmann::detail::exception& e) {
        if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
        return;
      }
      int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
      if (len == -1) {
        cerr << "发送添加好友消息失败 -> " << buffer << endl;
      } else {
        cout << "已发送好友申请" << endl;
      }
    } else {
      cout << "您填入的ID号是您自己的ID" << endl;
    }
  } else {
    cout << "您填入的ID号格式有误" << endl;
  }
}
void FriendAddResponse(int clientfd) {
  json js,response;
  js["msg_id"] = ADD_FRIEND_REQ_LIST_MSG;
  js["user_id"] = current_user.id();
  string buffer = js.dump();
  int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
  if (len == -1) {
    cerr << "申请管理列表获取失败 -> " << buffer << endl;
  } else {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk);
    if (!add_friend_req_list.empty()) {
      cout << "有人申请添加为您的好友..." << endl;
      auto req = add_friend_req_list.begin();
      while (req != add_friend_req_list.end()){
        cout << "[" << req->first << "]" << req->second << endl;
        cout << "是否通过? y/n" << endl;
        string input;
        for (int i = 0; i < 3; ++i) {
          getline(cin, input);
          if (input == "yes" || input == "y") {
            response["add_friend_resp_yes_ids"].push_back(req->first);
            break;
          } else if (input == "no" || input == "n") {
            break;
          }
        }
        ++req;
      }
      if (!response["add_friend_resp_yes_ids"].empty()) {
        response["user_id"] = current_user.id();
        response["user_name"] = current_user.user_name();
        response["msg_id"] = ADD_FRIEND_MSG;
        string buffer = response.dump();
        int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
        if (len == -1) {
          cerr << "发送处理申请消息失败 -> " << buffer << endl;
        } else {
          // std::unique_lock<std::mutex> lk(m);
          // cv.wait(lk);
          cout << "处理成功" << endl;
        }
      }
    } else {
      cout << "空" << endl;
    }
  }
}

void DeleteFriend(int clientfd) {
  cout << "移除好友..." << endl;
  cout << "请输入您要移除的好友ID:";
  string input_id;
  getline(cin, input_id);
  regex rgx("[0-9]+");
  if (std::regex_match(input_id, rgx)) {
    int friend_id = atoi(input_id.c_str());
    auto it = current_user_friends_list.find(friend_id);
    if (it != current_user_friends_list.end()) {
      json js;
      js["msg_id"] = DEL_FRIEND_MSG;
      js["id"] = current_user.id();
      js["friend_id"] = friend_id;
      string buffer;
      try {
        buffer = js.dump();
      } catch (nlohmann::detail::exception& e) {
        if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
        return;
      }
      int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
      if (len == -1) {
        cerr << "发送移除好友消息失败 -> " << buffer << endl;
      } else {
        // std::unique_lock<std::mutex> lk(m);
        // cv.wait(lk);
        cout << "处理成功" << endl;
      }
    } else {
      cout << "此ID非您的好友" << endl;
    }
  } else {
    cout << "您填入的ID号格式有误" << endl;
  }
}

void CreateGroup(int clientfd) {
  cout << "创建群组..." << endl;
  cout << "请输入您想要的群组名:";
  char group_name[50] = {0};
  cin.getline(group_name, 50);
  if (group_name[0] !='\0') {
    cout << "请输入群简介:";
    char group_desc[200] = {0};
    cin.getline(group_desc, 200);
    json js;
    js["msg_id"] = CREATE_GROUP_MSG;
    js["creator_id"] = current_user.id();
    js["group_name"] = group_name;
    js["group_desc"] = group_desc;  
    string buffer;
    try {
      buffer = js.dump();
    } catch (nlohmann::detail::exception& e) {
      if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
      return;
    }
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1) {
      cerr << "发送创建群组消息失败 -> " << buffer << endl;
    } else {
      sem_wait(&rwsem);
    }
  }
}

void CreateGroupResponse(const json& response_js) {
  if (response_js["errno"].get<int>() != 0) {
    cerr << "创建群组失败" << endl;
  } else {
    cout << "----------------------------------" << endl;
    cout << "创建群组成功!" << endl;
    cout << "请记住您创建的群组ID号, 对外分享此ID号,让更多人加入您的群组: "
         << response_js["group_id"].get<int>() << endl;
  }
}

void JoinInGroup(int clientfd) {
  cout << "加入群组..." << endl;
  cout << "请输入您要加入的群组ID:";
  char pwd[18] = {0};
  string input_id;
  getline(cin, input_id);
  regex rgx("[0-9]+");
  if (std::regex_match(input_id, rgx)) {
    int group_id = atoi(input_id.c_str());
    json js;
    js["msg_id"] = JOIN_IN_GROUP_MSG;
    js["user_id"] = current_user.id();
    js["group_id"] = group_id;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1) {
      cerr << "发送加入群组消息失败 -> " << buffer << endl;
    } else {
      std::unique_lock<std::mutex> lk(m);
      cv.wait(lk);
    }
  }
}

void GroupChat(int clientfd) {
  cout << "群组聊天..." << endl;
  cout << "请输入想要聊天的群组ID:";
  bool is_continue = false;
  string input_id;
  int group_id;
  getline(cin, input_id);
  regex rgx("[0-9]+");
  if (std::regex_match(input_id, rgx)) {
    group_id = atoi(input_id.c_str());
    do {
      char message[1024];
      cout << "发送内容:";
      cin.getline(message,1024);
      json js;
      try { 
        js["group_name"] = groups_id_and_name.at(group_id);
      } catch (std::out_of_range oor) {
        cout << oor.what() << ":" << "没有查询到对应的群组" << endl; 
      }
      if (!js["group_name"].empty()) {
        js["msg_id"] = GROUP_CHAT_MSG;
        js["user_id"] = current_user.id();
        js["user_name"] = current_user.user_name();
        js["group_id"] = group_id;
        js["msg"] = message;
        js["time"] = CurrentTime();
        string buffer;
        try {
          buffer = js.dump();
        } catch (nlohmann::detail::exception& e) {
          if (e.id == 316) cout << "字符编码有误,请检查输入" << endl;
          return;
        }
        int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
        if (len == -1) {
          cerr << "发送群组聊天消息失败 -> " << buffer << endl;
        } else {
          cout << "消息发送成功" << endl;
        }
        cout << "是否继续? y/n: ";
        string input;
        for (int i = 0; i < 3; ++i) {
          getline(cin, input);
          if (input == "yes" || input == "y") {
            is_continue = true;
            break;
          } else if (input == "no" || input == "n") {
            is_continue = false;
            break;
          }
        }
      }
    } while(is_continue);
  } else {
    cout << "您填入的ID号格式有误" << endl;
  }
}

void LeaveGroup(int clientfd) {
  cout << "退出群组..." << endl;
  cout << "请输入您要退出的群组ID:";
  string input_id;
  getline(cin, input_id);
  regex rgx("[0-9]+");
  if (std::regex_match(input_id, rgx)) {
    int group_id = atoi(input_id.c_str());
    auto it = current_user_groups_list.begin();
    while (it != current_user_groups_list.end()) {
      if (group_id != it->group_id()) {
        ++it;
      } else {
        json js;
        js["msg_id"] = LEAVE_GROUP_MSG;
        js["id"] = current_user.id();
        js["group_id"] = group_id;
        string buffer = js.dump();
        int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
        if (len == -1) {
          cerr << "发送退出群组消息失败 -> " << buffer << endl;
        } else {
          // std::unique_lock<std::mutex> lk(m);
          // cv.wait(lk);
          cout << "处理成功" << endl;
        }
        break;
      }
    }
    if (it == current_user_groups_list.end()) {
      cout << "您未加入该群组" << endl;
    }
  } else {
    cout << "您填入的ID号格式有误" << endl;
  }
}

void LoginOut(int clientfd) {
  json js;
  js["msg_id"] = LOGINOUT_MSG;
  js["id"] = current_user.id();
  string buffer = js.dump();
  int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
  if (len == -1) {
    cerr << "发送消息失败 -> " << buffer << endl;
  } else {
    is_main_menu_running = false;
  }
}

string CurrentTime() {
  auto time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  struct tm* ptm = localtime(&time);
  char date[60] = {0};
  snprintf(date, 60, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
  return std::string(date);
}

void RefreshRelatedLists(int clienfd) {
  json js;
  js["msg_id"] = REFRESHLISTS_MSG;
  js["user_id"] = current_user.id();
  string buffer = js.dump();
  int len = send(clienfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
  if (len == -1) {
    cerr << "发送消息失败 -> " << buffer << endl;
  } else {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk);
    cout << "刷新成功" << endl;
  }
}

