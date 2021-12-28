#ifndef CHAT_SERVER_MODEL_FRIEND_MODEL_H_
#define CHAT_SERVER_MODEL_FRIEND_MODEL_H_

#include <vector>
#include "user.h"
using std::vector;

class FriendModel {
 public:
  //
  void Insert(int user_id, int friend_id);
  //
  void Delete(int user_id, int friend_id);

  //
  vector<User> Query(int user_id);
};

#endif // CHAT_SERVER_MODEL_FRIEND_MODEL_H_