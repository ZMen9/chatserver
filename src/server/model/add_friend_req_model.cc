#include "add_friend_req_model.h"

#include "database.h"
using std::vector;
using std::pair;
void AddFriendReqModel::Insert(int user_id, int to_id) {
  char sql[256] = {""};
  std::snprintf(sql, 256, "insert into AddFriendReq values ('%d', '%d')",
                user_id, to_id);
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql);
  }
}

void AddFriendReqModel::Remove(int user_id, int to_id) {
  char sql[128] = {""};
  std::snprintf(sql, 128,
                "delete from AddFriendReq where sender_id=%d and to_id=%d",
                user_id, to_id);
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql);
  }
}

vector<pair<int, string>> AddFriendReqModel::Query(int user_id) {
  char sql[128] = {""};
  std::snprintf(sql, 128,
                "select u.id, u.user_name from User u inner join AddFriendReq "
                "A on A.sender_id = u.id where to_id = %d", user_id);
  vector<pair<int, string>> friend_req_vec{};
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        int sender_id = atoi(row[0]);
        string sender_name = row[1];
        friend_req_vec.push_back({sender_id, sender_name});
      }
      mysql_free_result(res);
      return friend_req_vec;
    }
  }
  return friend_req_vec;
}


