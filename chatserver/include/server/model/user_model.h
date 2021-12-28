// Copyright 2021
// License
// Author: ZeMeng Zheng
// This is the operation class of the User database table
#ifndef CHAT_SERVER_MODEL_USERMODEL_H_
#define CHAT_SERVER_MODEL_USERMODEL_H_

#include "user.h"

#include <string>
using std::string;
class UserModel {
 public: 
  bool Insert(User* user);
  User Query(int id);
  bool BoolQuery(int id);
  // 更新用户在线与否状态
  bool UpdateState(const User& user);
  // 重置所有用户的离线状态信息
  void ResetAllState();

};

#endif // CHAT_SERVER_MODEL_USERMODEL_H_