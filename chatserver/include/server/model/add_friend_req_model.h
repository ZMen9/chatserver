#ifndef CHAT_SERVER_MODEL_ADD_FRIEND_REQ_MODEL_H_
#define CHAT_SERVER_MODEL_ADD_FRIEND_REQ_MODEL_H_

#include <string>
#include <vector>
#include <utility>
#include "user.h"
class AddFriendReqModel {
 public:
  void Insert(int user_id, int to_id);
  void Remove(int user_id, int to_id);
  std::vector<std::pair<int, std::string>> Query(int user_id);
};


#endif // CHAT_SERVER_MODEL_ADD_FRIEND_REQ_MODEL_H_