#include "chatservice.hpp"
#include "public.hpp" //定义了消息id
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance() // 静态方法在类外地的时候就不需要再写static了
{
    static ChatService service; // 实例化一个单例对象 作为静态变量，生命周期和程序一样  只会在第一次调用的时候实例化，后续再调用该函数也不会了
    return &service;            // 当创建实例的时候就调用构造函数完成对_msgHandlerMap的初始化了
}

// 构造方法  完成构造消息以及对应回调映射的初始化
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEDN_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    //连接redis服务器
    if(_redis.connect())
    {
        //注册上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

// 处理登录业务   也就是说这里只管实现该业务逻辑即可，至于什么发生什么时候做，那是网络部分要处理的
/**
 * 同样的，在业务层这里也不要去写数据库的什么具体操作，所以这里也要把业务层和数据层进行解耦
 * 解耦的好处就在于一个模块改动的话不需要去把另一个模块逻辑都改动，比如突然不想使用mysql存储了，改换redis，那就只需要该数据层就行了，
 * 所以在这里不要出现任何mysql增删改查语句，这里只操作对象即可
 */
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int id = js["id"];
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd) // 如果登录密码与数据库里注册存储的账号对应密码匹配正确
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 该用户还没有登录，则登录成功

            // 登录成功，记录在线用户TCP连接信息
            // 在这里使用互斥锁 只有抢到了锁才能对该变量进行操作，不然就在这里进行等待
            // lock_guard是模板类，支持多种锁类型而且在异常时互斥锁也能被正确释放，封装了对std::mutex的锁定和解锁操作，在构造时自动加锁，在解析时自动解锁。但也自然不能手动解锁
            // lock_guard的构造函数接受一个互斥锁的引用，并自动获取锁，如果互斥锁已经被其他线程锁定，则当前线程会被阻塞，直到锁可用为止。
            // lock_guard的析构函数，会在对象生命周期结束时自动释放锁
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            } // 这样出了作用域就能自动解锁了


            //向redis服务器订阅通道，通道id就选用当前用户id就好了，这样哪个通道有消息就说明消息是发给该用户的
            _redis.subscribe(id);

            // 登录成功则刷新 用户状态信息
            user.setState("online");
            _userModel.updateState(user); // 数据库这边的并发操作不用管，那是由mysql来去保证的

            json response; // 这些是局部变量，每个线程栈都有自己的，不会冲突
            response["msgid"] = LOGGIN_MSG_ACK;
            response["errno"] = 0;         // 错误标识 因为响应消息不一定都是正确的，带一些额外的信息，作为消息的验证 接受到消息后先看此内容为0说明是正确响应，否则就不需要看其他字段了
            response["id"] = user.getId(); // 这样就封装好了json对象，主要携带了两个信息 作为返回
            response["name"] = user.getName();

            // 登录成功后，查询数据库里该用户是否有离线消息，有的话就一并发过去
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec; // json库是直接可以和c++容器直接进行序列化或反序列化，十分方便
                _offlineMsgModel.remove(id);  // 读取离线消息走后，应该把存储的离线消息清空
            }

            // 登录成功后，查询用户的好友信息，一并返回
            vector<User> uservec = _friendModel.query(id);
            if (!uservec.empty())
            {
                vector<string> vec2;
                for (User &user : uservec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 登录成功后，查询用户的群组信息，一并返回
            vector<Group> groupvec = _groupModel.queryGroups(id);
            if (!groupvec.empty())
            {
                vector<string> vec3;
                for (Group &group:groupvec)
                {
                    json js2;
                    js2["id"] = group.getId();
                    js2["groupname"] = group.getName();
                    js2["groupdesc"] = group.getDesc();

                    vector <string> usersvec;
                    
                    for(GroupUser &user:group.getUsers())
                    {
                        json js3;
                        js3["id"] = user.getId();
                        js3["name"] = user.getName();
                        js3["state"] = user.getState();
                        js3["role"] = user.getRole();
                        usersvec.push_back(js3.dump());
                    }
                    js2["users"] = usersvec;
                    vec3.push_back(js2.dump());
                }

                response["groups"] = vec3;
            }

            conn->send(response.dump()); // 将json对象序列化为字节流，通过网络发送回去
        }
    }
    else
    {
        // 该用户不存在或密码错误
        if (user.getId() == id)
        {
            // 说明用户存在，但是密码错误
            json response;
            response["msgid"] = LOGGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "password is invalid!";
            conn->send(response.dump());
        }
        else
        {
            json response;
            response["msgid"] = LOGGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "id is invalid!";
            conn->send(response.dump());
        }
    }
}

// 处理注册业务   注册的话填入登记两个字段即可 name password  由于已经对数据库的操作都已经封装好，这里只需要操作对象 不涉及底层的mysql语句
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    string name = js["name"];
    string pwd = js["password"];

    User user; // 先封装user表的一行信息
    user.setName(name);
    user.setPwd(pwd);

    bool state = _userModel.insert(user); // 根据user的信息封装好mysql语句，通过db类操作登录进入服务器的正确的数据库chat，并执行mysql语句
    if (state)
    {
        // 如果注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;         // 错误标识 因为响应消息不一定都是正确的，带一些额外的信息，作为消息的验证 接受到消息后先看此内容为0说明是正确响应，否则就不需要看其他字段了
        response["id"] = user.getId(); // 这样就封装好了json对象，主要携带了两个信息 作为返回
        conn->send(response.dump());   // 将json对象序列化为字节流，通过网络发送回去
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump()); // 将json对象序列化为字节流，通过网络发送回去
    }
}

// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int toid = js["toid"].get<int>();

    {
        // 查找聊天对象是否在线
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid); // 根据键直接找
        if (it != _userConnMap.end())
        {
            // 如果找到了，说明对方和自己登录在同一台服务器，那么直接就可以转发消息，则服务器主动推送消息给toid用户
            it->second->send(js.dump()); // 将消息原封不动发给接收方的Tcp
            return;
        }
    }

    //如果没找到，要么对方在别的服务器上登录，要么就是不在线（即不在任何一台服务器上登录）
    //这时候查tcp连接时不行的，要查数据库看看对方是否在线
    User user = _userModel.query(toid);
    if(user.getState() == "online")
    {
        //对方登录在别的服务器上
        _redis.publish(toid,js.dump()); //那就向redis通道发布消息  由redis发布-订阅机制实现服务器集群的通信
        return;
    }

    // 如果对方不在线，则向数据库存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1,name,desc);//组id在表内时自增生成的，用不上
    if(_groupModel.createGroup(group))  //对allgroup操作
    {
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"creator"); //初始创建，当然只有群主一人  对groupuser操作
    }

}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid);//userid所在指定组的所有其他用户id
    
    lock_guard<mutex> lock(_connMutex);//确保线程安全
    for(int id:useridVec)
    {
        auto it = _userConnMap.find(id);//查找每个id对应的Tcp连接
        if(it!=_userConnMap.end())
        {
            //如果找到，说明对方与自己登录在同一台服务器上，则转发群消息
            it->second->send(js.dump());
        }
        else  //如果没找到tcp连接，那么由两种可能，要么对方登录在别的服务器，要么根本不在线
        {
            User user = _userModel.query(id);
            if(user.getState() == "online") //如果登录在别的服务器上，则向redis的通道发布消息
            {
                _redis.publish(id,js.dump());
            }
            else //如果根本就不在线
            {
                 //存储离线消息
                _offlineMsgModel.insert(id,js.dump()); 
            }

        }
    }
}


//处理正常注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js,Timestamp)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //用户下线，那么在redis中就要取消订阅
    _redis.unsubscribe(userid);

    //重置用户的状态信息
    User user(userid,"","","offline");
    _userModel.updateState(user);

}

// 获取消息对应的处理器  这样通过消息id就可以获得对应的业务处理回调
MsgHandler ChatService::getHandler(int msgid)
{
    // 这里也可以实现 记录错误日志，msgid也没有找到对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 没有找到的话，返回一个默认的空处理器，执行空操作，就是什么也不做
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid]; // 以免没有这种这种id就往里面乱添加
    }
}

// 处理客户端异常注销
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    // 要做两件事，一是根据这个conn去找到用户id，从而可以根据id去把数据库里的用户state修改为offline  二就是把conn在在线映射表中删除
    // 同样，操作_userConnMap（在线用户-连接映射表）需要注意线程安全，比如有的用户可能正在登录
    User user;
    { // 对公共资源的_userConnMap操作需要确保线程安全
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户异常退出下线，当然也要在redis中取消订阅
    _redis.unsubscribe(user.getId());


    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user); // 根据id去重新设置state
    }
}

// 服务器异常后，业务重置方法
void ChatService::reset()
{
    // 把所有Online用户的状态重置为offline
    _userModel.resetState();
}



//从redis服务器中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid,string msg)  //当订阅的通道收到了消息才会调用此函数
{
    
    lock_guard lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it != _userConnMap.end()) //其实通道能收到消息，那是百分百在线的了  只不过要根据id找到对应的tcp连接
    {
        it->second->send(msg);//向该用户的tcp连接转发该消息
        return;
    }

    //当然，也有可能在分布和收到消息的过程中就突然碰巧下线了，这时候就直接存储离线消息即可
    _offlineMsgModel.insert(userid,msg);
}

