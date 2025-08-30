#include "usermodel.hpp"
#include "db.h" //操作数据库的头文件

#include <iostream>
using namespace std;

// User表的增加
bool UserModel::insert(User &user)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        if (mysql.update(sql)) // 传入mysql语句
        {
            // 获取插入成功的用户数据生成的主键id  由于表的id是自增的，所以获取插入后的id值在设置到user映射里就行了
            user.setId(mysql_insert_id(mysql.getConnection())); // mysql_insert_id能返回上一步inser操作产生的id值
            return true;
        }
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        MYSQL_RES *res = mysql.query(sql); // 传入mysql语句
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res); // 将行全拿出来
            if (row != nullptr)
            {
                // 里面有数据的话
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res); // 注意要释放资源，mysql.query(sql)语句肯定是申请了资源的
                return user;
            }
        }
    }
    return User(); // 返回默认值 -1 ""  ""  "offline"
}

// 刷新用户状态信息
bool UserModel::updateState(User user)
{
    // 1、组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = '%d'",
            user.getState().c_str(), user.getId());

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        if (mysql.update(sql)) // 传入mysql语句
        {
            return true;
        }
    }
    return false;
}


// 重置用户状态信息
void UserModel::resetState()
{
    // 1、组装sql语句
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    // 定义一个MySQL对象
    MySQL mysql;
    if (mysql.connect()) // 连接登录进入正确的数据库
    {
        mysql.update(sql);// 传入mysql语句 
    }
   
}