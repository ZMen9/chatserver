#ifndef CHAT_SERVER_MODEL_GROUP_H_
#define CHAT_SERVER_MODEL_GROUP_H_

#include <vector> 
#include <string>
#include "group_users.h"
using std::string;
using std::vector;

class Group {
 public:
  Group(int group_id = -1, string group_name = "", string desc = "") 
    : group_id_(group_id), group_name_(group_name), desc_(desc) {}
  
  void set_group_id(int id) { this->group_id_ = id; }
  void set_group_name(string group_name) { this->group_name_ = group_name; }
  void set_desc(string desc) { this->desc_ = desc; }
  
  int group_id() const { return this->group_id_; }
  string group_name() const { return this->group_name_; }
  string desc() const { return this->desc_; }
  vector<GroupUser>& group_users() const {return this->group_users_; }

 private: 
  int group_id_;
  string group_name_;
  string desc_;
  mutable vector<GroupUser> group_users_{};
};

#endif //CHAT_SERVER_MODEL_GROUP_H_

