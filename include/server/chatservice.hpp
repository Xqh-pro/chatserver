#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>  //要确保线程安全

using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "redis.hpp"  //实现跨服务器聊天
#include "usermodel.hpp" //抽象了对数据库的user表的操作
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
using json = nlohmann::json;
//处理消息的事件回调方法类型
using MsgHandler = std::function<void (const TcpConnectionPtr &conn, json &js,Timestamp)>;

/**
 * 聊天服务器-业务类
 * 
 * 此模块负责实现业务逻辑
 */
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    /**
     * 业务方法
     */
    //处理登录业务  
    void login(const TcpConnectionPtr &conn, json &js,Timestamp);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js,Timestamp);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js,Timestamp);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js,Timestamp);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js,Timestamp);
    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js,Timestamp);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js,Timestamp);
    //处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js,Timestamp);

    //从redis服务器中获取订阅的消息
    void handleRedisSubscribeMessage(int,string);

    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);
    //服务器异常后，业务重置方法
    void reset();

private:
    ChatService();//构造函数私有化
    //要给msgid映射一个业务处理的回调  这样就解码网络和业务代码  消息与枚举类型挂钩，通过该int值确定执行什么业务逻辑
    //此事先都定好了的，不会有什么变化
    unordered_map<int, MsgHandler> _msgHandlerMap; //存储每个消息id 以及其对应的业务事件处理方法

    //数据操作类
    UserModel _userModel;
    //数据操作类
    OfflineMsgModel _offlineMsgModel;
    //数据操作类
    FriendModel _friendModel;
    //数据操作类
    GroupModel _groupModel;

    //redis操作对象
    Redis _redis;

    //定义互斥锁变量，用于控制对共享资源_userConnMap的访问(即确保线程安全)
    mutex _connMutex;

    //存储在线用户的通信连接（在线用户-连接映射表）  因为服务器端除了被动接受消息外，还需要去根据消息目标去主动向目标推送消息，那就需要存储所有在线用户通信连接才行了
    //注意随着用户不断地连接与断开，其会动态的变化  可以使用智能指针或互斥锁 来确保线程安全
    unordered_map<int,TcpConnectionPtr> _userConnMap;//用户的id好映射到Tcp指针（连接服务器成功的话，该指针就不会为空）
    
};

#endif