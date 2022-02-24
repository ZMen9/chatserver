#include "user_model.h"

#include <iostream>

#include "database.h"
bool UserModel::Insert(User* user) {
  char sql[256] = {""};
  std::snprintf(
      sql, 256,
      "insert into User(user_name, password, state) values ('%s', '%s', '%s')",
      user->user_name().c_str(), user->password().c_str(), user->state().c_str());

  MySql mysql;
  if (mysql.Connect()) {
    if (mysql.Update(sql)) {
      user->set_id(mysql_insert_id(mysql.current_connection()));
      return true;
    }
  }
  return false;
}

User UserModel::Query(int id) {
  char sql[128] = {""};
  std::snprintf(sql, 128, "select * from User Where id = %d", id);
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row != nullptr) {
        User user;
        user.set_id(std::atoi(row[0]));
        user.set_user_name(row[1]);
        user.set_password(row[2]);
        user.set_state(row[3]);
        mysql_free_result(res);
        return user;
      }
    }
  }
  return User{};
}

bool UserModel::BoolQuery(int id) {
  char sql[128] = {""};
  std::snprintf(sql, 128, "select * from User Where id = %d", id);
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row != nullptr) {
        return true;
      }
    }
  }
  return false;
}

bool UserModel::UpdateState(const User& user) {
  char sql[128] = {""};
  std::snprintf(sql, 128, "update User set state = '%s' where id = %d",
                user.state().c_str(), user.id());
  MySql mysql;
  if (mysql.Connect()) {
    if (mysql.Update(sql)) {
      return true;
    }
  }
  return false;
}
void UserModel::ResetAllState() {
  char sql[128] = {""};
  std::snprintf(sql, 128, 
                "update User set state = 'offline' where state = 'online'");
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql); 
  }
}