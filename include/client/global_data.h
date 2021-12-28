#ifndef CHAT_CLIENT_LOCAL_DATA_H_
#define CHAT_CLIENT_LOCAL_DATA_H_
#include <vector>
#include <atomic>
#include <map>
#include <utility>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <semaphore.h>

#include "user.h"
#include "group.h"
using std::vector;
using std::map;
using std::pair;

extern bool is_main_menu_running;
extern sem_t rwsem;
extern std::atomic_bool is_login_success;
extern std::mutex m;
extern std::condition_variable cv;
extern User current_user;
// 保存有关所有信息在本地客户端程序的两个记录块
extern map<int,User> current_user_friends_list;
extern vector<Group> current_user_groups_list;
extern map<int,string> groups_id_and_name;
extern vector<pair<int, string>> add_friend_req_list;

#endif