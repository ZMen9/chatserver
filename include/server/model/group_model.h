#ifndef CHAT_SERVER_MODEL_GROUP_MODEL_H_
#define CHAT_SERVER_MODEL_GROUP_MODEL_H_

#include <string>
#include <vector>

#include "group.h"

class GroupModel {
 public:
  // 创建群组
  bool CreateGroup(Group* group);
  // 查询群组是否存在
  bool QueryGroupExisting(int group_id);
  
  Group QueryOneGroupAndMembers(int user_id, int group_id);
  // 加入群组
  void JoinInGroup(int user_id, int group_id, const string& role);
  // 查询用户所在所有群组以及群员的信息
  vector<Group> QueryGroupsAndUsers(int user_id);
 
  // 查询所在某个群的其他组员id号
  vector<int> QueryGroupOtherUersId(int user_id, int group_id);
  
  bool DeleteMember(int user_id,int friend_id);
  
};



#endif // CHAT_SERVER_MODEL_GROUP_MODEL_H_