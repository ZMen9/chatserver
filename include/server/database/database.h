#ifndef CHAT_SERVER_DATABASE_H_
#define CHAT_SERVER_DATABASE_H_

#include <mysql/mysql.h>
#include <string>
using std::string;
class MySql {
 public:
  MySql();
  ~MySql();

  bool Connect();
  // Update()以及Query()的参数类型不该是std::string&引用类型
  bool Update(string sql);
  MYSQL_RES* Query(string sql);
  MYSQL* current_connection();

 private:
  MYSQL* conn_;
};

#endif  // CHAT_SERVER_DATABASE_H_