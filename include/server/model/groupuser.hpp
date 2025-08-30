#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

//群组用户，相比user多了一个在群组内的role角色信息，比如到底是群组管理员，还是普通组员
//所以直接从user类上继承
class GroupUser:public User
{
private: 
    string role;
public:
    void setRole(string role) {this->role = role;}
    string getRole(){return this->role;}
};


#endif