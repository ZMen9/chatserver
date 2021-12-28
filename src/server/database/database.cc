#include "database.h"
#include <muduo/base/Logging.h>

static string host = "127.0.0.1";
static string user = "chat";
static string password = "chat";
static string dbname = "chat";

MySql::MySql() { conn_ = mysql_init(nullptr); }

MySql::~MySql() {
  if (conn_ != nullptr) {
    mysql_close(conn_);
  }
}

bool MySql::Connect() {
  MYSQL* connection =
      mysql_real_connect(conn_, host.c_str(), user.c_str(), password.c_str(),
                         dbname.c_str(), 3306, nullptr, 0);
  if (connection != nullptr) {
    mysql_query(conn_, "set names utf8");
    LOG_INFO << "连接数据库成功... ";
  } else {
    LOG_INFO << "连接数据库失败... ";
  }
  return connection;
}

bool MySql::Update(string sql) {
  if (mysql_query(conn_, sql.c_str())) {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ";" << sql << " - 更新失败";
    return false;
  } else {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ";" << sql << " - 更新成功";
    return true;
  }
}

MYSQL_RES* MySql::Query(string sql) {
  if (mysql_query(conn_, sql.c_str())) {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ";" << sql << " - 查询失败";
    return nullptr;
  } else {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ";" << sql << " - 查询成功";
    return mysql_use_result(conn_);
  }
}

MYSQL* MySql::current_connection() {
  return conn_;
}
