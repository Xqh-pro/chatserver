#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv){

    if(argc<3)
    {
        cerr<<"comand invalid! example: ./  ChatServer 192.168.0.101 6000"<<endl;
    }

    //解析通过命令行参数传递的ip地址和端口号port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,resetHandler);
  
    EventLoop loop; // epoll
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "Chatserver");
    server.start(); // 启动服务   将listenfd  epoll_ctl添加到epoll上
    loop.loop();    // 调研epoll_wait，以堵塞方式等待 新用户的连接，已连接用户的读写事件等
    // 没有loop调用的话，程序会立即退出结束了，所以前面的代码执行好了只是完成了配置，需要在这里调用loop循环堵塞当前主线程，
    
    return 0; 

}