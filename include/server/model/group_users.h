#ifndef CHAT_SERVER_MODEL_GROUP_USERS_H_
#define CHAT_SERVER_MODEL_GROUP_USERS_H_

#include <string>
#include "user.h"
using std::string;
class GroupUser : public User {
 public:
  void set_role(const string& role) { this->role_ = role; }
  string role() const { return this->role_; }

 private:
  string role_;
};

#endif //CHAT_SERVER_MODEL_GROUP_USERS_H_