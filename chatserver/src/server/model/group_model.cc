#include "group_model.h"
#include "database.h"

bool GroupModel::CreateGroup(Group* group) {
  char sql[512] = {0};
  std::snprintf(sql, 512,
                "insert into Allgroup(groupname,groupdesc) values('%s','%s')",
                group->group_name().c_str(), group->desc().c_str());
  MySql mysql;
  if (mysql.Connect()) {
    if (mysql.Update(sql)) {
      group->set_group_id(mysql_insert_id(mysql.current_connection()));
      return true;
    }
  }
  return false;
}

bool GroupModel::QueryGroupExisting(int group_id) {
  char sql[128] = {0};
  std::snprintf(sql, 128, "select id from Allgroup where id = %d", group_id);
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row = mysql_fetch_row(res);
      if (row != nullptr) {
        return true;
      }
    }
  }
  return false;
}
Group GroupModel::QueryOneGroupAndMembers(int user_id,int group_id) {
  char sql[256] = {0};
  std::snprintf(
      sql, 256,
      "select A.id, A.groupname, A.groupdesc from Allgroup A inner "
      "join GroupUser G on G.groupid = A.id where G.userid = %d and A.id = %d",
      user_id, group_id);
  MySql mysql;
  Group group;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql); 
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        group.set_group_id(atoi(row[0]));
        group.set_group_name(row[1]);
        group.set_desc(row[2]);
      }
      mysql_free_result(res);
    }
  }
  std::snprintf(
      sql, 256,
      "select u.id, u.user_name, u.state, g.grouprole from User u inner join "
      "GroupUser g on g.userid = u.id where g.groupid = %d",
      group_id);
  MYSQL_RES* res = mysql.Query(sql);
  if (res != nullptr) {
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
      GroupUser group_user;
      group_user.set_id(atoi(row[0]));
      group_user.set_user_name(row[1]);
      group_user.set_state(row[2]);
      group_user.set_role(row[3]);
      group.group_users().push_back(group_user);
    }
    mysql_free_result(res);
  }
  return group;
}

void GroupModel::JoinInGroup(int user_id, int group_id, const string& role) {
  char sql[128] = {0};
  std::snprintf(sql, 128,
                "insert into GroupUser values('%d','%d','%s')", group_id, user_id, role.c_str());
  MySql mysql;
  if (mysql.Connect()) {
    mysql.Update(sql); 
  }
}

vector<Group> GroupModel::QueryGroupsAndUsers(int user_id) {
  char sql[256] = {0};
  std::snprintf(sql, 256,
                "select A.id, A.groupname, A.groupdesc from Allgroup A inner "
                "join GroupUser G on G.groupid = A.id where G.userid = %d",
                user_id);

  MySql mysql;
  vector<Group> groups_vec;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql); 
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        Group group;
        group.set_group_id(atoi(row[0]));
        group.set_group_name(row[1]);
        group.set_desc(row[2]);
        groups_vec.push_back(group);
      }
      mysql_free_result(res);
    }
  }
  for (Group& group : groups_vec) {
    std::snprintf(
        sql, 256,
        "select u.id, u.user_name, u.state, g.grouprole from User u inner join "
        "GroupUser g on g.userid = u.id where g.groupid = %d",
        group.group_id());
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupUser group_user;
        group_user.set_id(atoi(row[0]));
        group_user.set_user_name(row[1]);
        group_user.set_state(row[2]);
        group_user.set_role(row[3]);
        group.group_users().push_back(group_user);
      }
      mysql_free_result(res);
    }
  }
  return groups_vec;
}

vector<int> GroupModel::QueryGroupOtherUersId(int user_id, int group_id) {
  char sql[128] = {0};
  std::snprintf(
      sql, 128,
      "select userid from GroupUser where groupid = %d and userid != %d",
      group_id, user_id);
  vector<int> users_id_vec;
  MySql mysql;
  if (mysql.Connect()) {
    MYSQL_RES* res = mysql.Query(sql);
    if (res != nullptr) {
      MYSQL_ROW row;
      while ((row = mysql_fetch_row(res)) != nullptr) {
        users_id_vec.push_back(atoi(row[0]));
      }
      mysql_free_result(res);
    } 
  }
  return users_id_vec;
}

bool GroupModel::DeleteMember(int user_id,int group_id) {
  char sql[128] = {""};
  std::snprintf(sql, 128,
                "delete from GroupUser where userid=%d and groupid=%d", user_id,
                group_id);
  MySql mysql;
  if (mysql.Connect()) {
    if (mysql.Update(sql)) {
      return true;
    }
  }
  return false;
}
