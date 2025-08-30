#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>
using namespace std;
/**
 * offlinemessage表的数据操作类
 * 该表存储了用户离线后仍接受到消息
 * 该类提供对此表的操作方法
 * 这些方法就和业务是不相关的，只是针对表的增删改查的封装
 */
class OfflineMsgModel
{
public:
    //存储用户的离线消息
    void insert(int userid, string msg);

    //删除用户的离线消息
    void remove(int userid);

    //查询用户的离线消息  用户的离线消息可能不止一个，可能有多个，所以使用一个容器来接受作为返回值
    vector<string> query(int userid);

};

#endif