#ifndef CHATSERVER_H
#define CHATSERVER_H

/*
muduo网络库给用户提供了两个主要的类
1：TcpServer,用于编写服务器程序的
2：TcpClient，用于编写客户程序的
##### 注意链接的顺序不要搞错：g++ -o server muduo_server.cpp -lmuduo_net -lmuduo_base -lpthread
epoll+线程池
好处：能把网络I/O的代码和业务代码封开，  结果muduo网络库封装之后，用户只需要把精力放在使用网络库完成业务代码即可，至于网络i/o操作代码就直接由网络库帮忙封装好了
                                                                                    业务代码：用户的链接和断开  用户的可读写事件 我们用户只需要关注这两件事怎么做就行了，至于什么时候发生这些事件，由网络库完成监听上报
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;


//聊天服务器主类
class ChatServer{
public:
     /*TcpServer需要的参数
    EventLoop* loop,
    const InetAddress& listenAddr,
    const string& nameArg,
     */
    ChatServer(EventLoop* loop,
    const InetAddress& listenAddr,
    const string& nameArg);

    //启动服务
    void start();

private:
    TcpServer _server;//组合的muduo库，实现服务器功能的类对象
    EventLoop * _loop; //指向事件循环的指针

    //上报链接相关的回调函数
    void onConnection(const TcpConnectionPtr&);
    //上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr&,  //连接
                        Buffer* ,   //缓冲区
                        Timestamp );
};


#endif