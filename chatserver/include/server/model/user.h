// Copyright 2021
// License
// Author: ZeMeng Zheng
// This is the ORM class of the User database table
#ifndef CHAT_SERVER_MODEL_USER_H_
#define CHAT_SERVER_MODEL_USER_H_

#include <string>
using std::string;

class User {
 public:
  User(int id = -1, string user_name = "", string password = "",
       string state = "offline")
      : id_(id), user_name_(user_name), password_(password), state_(state){};

  void set_id(int id) { id_ = id; }
  void set_user_name(const string& user_name) { user_name_ = user_name; }
  void set_password(const string& password) { password_ = password; }
  void set_state(const string& state) { state_ = state; }

  int id() const { return id_; }
  string user_name() const { return user_name_; }
  string password() const { return password_; }
  string state() const { return state_; }

 private:
  int id_;
  string user_name_;
  string password_;
  string state_;
};

#endif  // CHAT_SERVER_MODEL_USER_H_