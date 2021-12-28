#include "friend_model.h"
#include "database.h"
void FriendModel::Insert(int user_id, int friend_id) {
  char sql[128] = {""};
  std::snprintf(sql, 128, "insert into Friend values(%d, %d),(%d, %d)", user_id,
                friend_id, friend_id, user_id);
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql); 
  }
}
void FriendModel::Delete(int user_id,int friend_id) {
  char sql[128] = {""};
  std::snprintf(
      sql, 128,
      "delete from Friend where userid in (%d, %d) and friendid in(%d,%d);",
      user_id, friend_id, friend_id, user_id);
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql);
  }
}

vector<User> FriendModel::Query(int user_id) {
  char sql[128] = {""};
  std::snprintf(sql, 128,
                "select u.id, u.user_name, u.state from User u inner join "
                "Friend f on f.friendid = u.id where f.userid = %d",
                user_id);
  vector<User> friend_vec{};
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        User user;
        user.set_id(atoi(row[0]));
        user.set_user_name(row[1]);
        user.set_state(row[2]);
        friend_vec.push_back(user);
      }
      mysql_free_result(res);
      return friend_vec;
    }
  }
  return friend_vec;
}
