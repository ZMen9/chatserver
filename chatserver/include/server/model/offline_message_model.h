#ifndef CHAT_SERVER_MODEL_OFFLINE_MESSAGE_MODEL_H_
#define CHAT_SERVER_MODEL_OFFLINE_MESSAGE_MODEL_H_

#include <string>
#include <vector>

class OfflineMsgModel {
 public:
  void Insert(int user_id, const std::string& msg);

  void Remove(int user_id);

  std::vector<std::string> Query(int user_id);
};

#endif // CHAT_SERVER_MODEL_OFFLINE_MESSAGE_MODEL_H_
