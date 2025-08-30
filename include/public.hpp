#ifndef PUBLIC_H
#define PUBLIC_H

/**
 * server和client的公共文件
 */


 enum EnMsgType{
    LOGIN_MSG = 1,  //1  登录消息id
    LOGGIN_MSG_ACK, //2 登录响应
    LOGINOUT_MSG,    //3 注销消息id
    REG_MSG,        //4  注册消息Id
    REG_MSG_ACK,    //5  注册响应
    ONE_CHAT_MSG,    //6  一对一聊天消息id
    ADD_FRIEDN_MSG,  //7  添加好友消息id

    CREATE_GROUP_MSG, //8 创建群组消息id
    ADD_GROUP_MSG,    //9 加入群组消息id
    GROUP_CHAT_MSG,   //10 群组聊天消息id


 };

#endif