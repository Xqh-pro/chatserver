#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系 不在本地存储，好友关系应该在客户端存储
void FriendModel::insert(int userid, int friendid)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d,'%d')",
            userid,friendid);

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        mysql.update(sql); // 传入mysql语句  
    }

}



//返回用户的好友列表
vector<User> FriendModel::query(int userid)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d", userid);

    vector<User> vec;
    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        MYSQL_RES *res = mysql.query(sql); // 传入mysql语句
        if (res != nullptr)
        {

            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr)  // 注意这里用户userid可能有很多行离线消息
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);//释放动态申请的资源
            return vec;
        }
    }
    return vec;
}