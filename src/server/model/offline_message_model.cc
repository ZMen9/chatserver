#include "offline_message_model.h"


#include "database.h"

using std::string;
using std::vector;

void OfflineMsgModel::Insert(int user_id, const string& msg) {
  char sql[256] = {""};
  std::snprintf(sql, 256, "insert into OfflineMessage values ('%d', '%s')",
                user_id, msg.c_str());
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql);
  }
}

void OfflineMsgModel::Remove(int user_id) {
  char sql[128] = {""};
  std::snprintf(sql, 128, "delete from OfflineMessage where userid=%d",
                user_id);
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql);
  }
}


vector<string> OfflineMsgModel::Query(int user_id) {
  char sql[1024] = {""};
  std::snprintf(sql, 1024, "select message from OfflineMessage where userid=%d",
                user_id);
  vector<string> msg_vec;
  
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        msg_vec.push_back(row[0]);
      }
      mysql_free_result(res);
      return msg_vec;
    }
  }
  return msg_vec;
}