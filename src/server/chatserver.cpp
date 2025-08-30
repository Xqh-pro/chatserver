
#include "chatserver.hpp"  //定义了网络模块
#include "json.hpp"
#include "chatservice.hpp"  //定义了消息类型对应的业务逻辑操作模块

#include <functional>
#include <iostream>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 基于muduo网络库开发服务器程序
// 1.组合TcpServer对象
// 2.创建EventLoop事件循环对象的指针
// 3.明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
// 4.再当前服务器类的构造函数中，注册处理链接的回调函数和处理读写事件的回调函数
// 5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程

// 初始化服务器聊天对象
ChatServer::ChatServer(EventLoop *loop,               // 事件循环
                       const InetAddress &listenAddr, // ip地址+端口号
                       const string &nameArg)         // 服务器的名称
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 给服务器注册用户链接的创建和断开回调
    // 传入一个函数对象 onConnection是自定义的成员函数，它的调用需要对象实例，不能直接传给setConnectionCallback
    //  void setConnectionCallback(const ConnectionCallback& cb)  cb是一个函数对象 要求为std::function<void (const TcpConnectionPtr&)> 显然要求为一个参数
    // 但是我们的onConnection成员函数的形参除了TcpConnectionPtr&之外，隐含了this指针形参,这样就是不符合要求的俩参数，绑定后生成一个兼容void (const TcpConnectionPtr&)的函数对象
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 绑定this 这样就解决了成员函数不能直接作为回调函数的问题  占位符_1表示新函数对象接受一个参数

    // 再给服务器注册用户读写事件回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3)); // 占位符表示回调函数需要传入三个参数

    // 设置服务器端的线程数量  1个io线程   3个worker线程
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start(); // 启动TcpServer 开始监听并处理客户端的连接请求
}

// 连接事件监听
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{ // 非静态成员函数有一个隐式this指针参数，用于访问对象实例。但普通函数指针或回调机制无法自动提供this

    if(!conn->connected()){//如果断开连接
        ChatService::instance()->clientCloseException(conn); //如果客户端是异常退出的话，不应该直接关闭，应该做一些处理
        conn->shutdown(); //关闭Tcp连接
    }
}

// 读写事件监听
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 连接
                           Buffer *buffer,               // 缓冲区
                           Timestamp time)               // 接收到数据的时间信息
{ 
    string buf = buffer->retrieveAllAsString(); // 取出缓冲区的数据并转化为一个string
    json js = json::parse(buf); //数据的反序列化   大家需要统一按照josn字符串格式来传输消息
    //解耦网络模块的代码和业务模块的代码
    //所这里这里只做抽象，没有任何业务的具体方法调用  只负责获取时间处理器来回调就行了。具体的业务逻辑由业务模块完成
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); //获取该消息的事件处理器
    msgHandler(conn,js,time);  //回调消息绑定好的事件处理器，来去执行相应的业务处理
    //不管后续业务怎么改，这里的逻辑都不需要改动
}


