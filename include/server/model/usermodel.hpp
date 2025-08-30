#ifndef USERMODEL_H
#define USERMODEL_H

/**
 * user表的数据操作类
 * 提供对表的操作方法
 * 这些方法就和业务是不相关的，只是针对表的增删改查的封装
 */

 #include "user.hpp"

class UserModel
{
public:
    //User表的增加
    bool insert(User &user);

    //根据用户号码查询用户信息
    User query(int id);

    //刷新用户状态信息
    bool updateState(User user);

    //重置用户状态信息
    void resetState();
    
};

#endif