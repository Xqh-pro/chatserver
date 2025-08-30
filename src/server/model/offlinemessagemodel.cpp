#include "offlinemessagemodel.hpp"
#include "db.h"  //这些方法本质是要对数据库进行操作


//存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d,'%s')",
            userid,msg.c_str());

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        mysql.update(sql); // 传入mysql语句  
    }

    return ;
}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d",
            userid);

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        mysql.update(sql); // 传入mysql语句  
    }

    return ;
}

//查询用户的离线消息  用户的离线消息可能不止一个，可能有多个，所以使用一个容器来接受作为返回值
vector<string> OfflineMsgModel::query(int userid)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);

    vector<string> vec;
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
                vec.push_back(row[0]);
            }
            mysql_free_result(res);//释放动态申请的资源
            return vec;
        }
    }
    return vec;
}