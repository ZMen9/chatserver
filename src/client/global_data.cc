#include "global_data.h"
bool is_main_menu_running = false;
sem_t rwsem;
std::atomic_bool is_login_success{false};
std::mutex m;
std::condition_variable cv;
User current_user;
// 保存有关所有信息在本地客户端程序的两个记录块
map<int, User> current_user_friends_list;

vector<Group> current_user_groups_list;
map<int,string> groups_id_and_name;
vector<pair<int, string>> add_friend_req_list;