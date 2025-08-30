#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp" //要联合user表将好友的更多信息返回，因为friend表只存储了用户id和好友id
#include <vector>
using namespace std;

/**
 * firend表的数据操作类
 * 该表维护用户的好友列表
 * 该类提供对此表的操作方法
 * 这些方法就和业务是不相关的，只是针对表的增删改查的封装
 */

class FriendModel
{
public:
    //添加好友关系 不在本地存储，好友关系应该在客户端存储
    void insert(int userid, int friendid);

    //返回用户的好友列表
    vector<User> query(int userid);

};

#endif