#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <functional>
#include <unordered_map>

#include <semaphore.h>  //使用linux的信号量
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"


//记录当前系统登录的用户信息
User g_currentUser;
//记录当前登录用户的   好友列表
vector<User> g_currentUserFriendList;
//记录当前登录用户的   群组列表
vector<Group> g_currentUserGroupList;

//控制聊天页面程序
bool isMainMenuRunning = false;

//定义读写线程间通信
sem_t rwsem;
//记录登录状态是否成功
atomic_bool g_isLoginSuccess{false};


//显示当前登录成功用户的基本信息
void showCurrentUserData();

//接受线程
void readTaskHandler(int clientfd);
//获取系统时间--用于聊天时的时间信息
string getCurrentTime();
//主聊天页面程序
void mainMenu(int clientfd);




//聊天客户端程序实现，main线程用作发送线程，子线程用作接受线程 不发也不影响接受
int main(int argc, char**argv)
{
    if(argc<3)
    {
        cerr<<"command invalid! example: ./ChatClient 192.168.0.101 6000"<<endl;
        exit(-1);
    }
    //解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket  
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == clientfd)
    {
        cerr<<"socket create error"<<endl;
        exit(-1);
    }

    //填写绑定client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //client和server进行连接
    if(-1 == connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)))
    {
        cerr<<"connect server error"<<endl;
        close(clientfd);//关闭套接字
        exit(-1);
    }

    //初始化主线程和子线程通信用的信号量
    sem_init(&rwsem, 0,0); //一个进程内的两个线程通信

    //tcp连接服务器成功，启动接受子线程专门用来接收来自服务器的消息
    std::thread readTask(readTaskHandler, clientfd);
    readTask.detach();
        

    //main线程用于接收用户输入，负责发送数据
    for(;;)
    {
        //显示首页面菜单 登录、注册、退出
        cout<<"=============================="<<endl;
        cout<<"1.login"<<endl;
        cout<<"2.register"<<endl;
        cout<<"3.quit"<<endl;
        cout<<"=============================="<<endl;
        cout<<"choice:";
        int choice = 0;
        cin>>choice;
        cin.get();//读掉缓冲区残留的回车

        switch(choice)
        {
            case 1://login业务
            {
                int id=0;
                char pwd[50] ={0};
                cout<<"userid:";
                cin>>id;
                cin.get();//读掉缓冲区残留的回车
                cout<<"userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                g_isLoginSuccess = false;
                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                //在这里要等待子线程接受服务端响应消息  接受到了会通知本主线程起来继续处理 看看是登录成功还是失败

                if(len == -1)
                {
                    cerr<<"send login msg error:"<<request<<endl;
                }
                
                sem_wait(&rwsem);//等待信号量，由子线程接受到服务器响应消息处理后通知
                if(g_isLoginSuccess) //唤醒后判断是否登录成功
                {
                    //登录成功，进入聊天交互页面
                    isMainMenuRunning = true;//进入主菜单循环交互页面
                    mainMenu(clientfd);
                }

            }
            break;
            case 2://注册业务
            {
                char name[50]={0};
                char pwd[50]={0};
                cout<<"username:";
                cin.getline(name,50);
                cout<<"userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();//序列化成字符串

                int len = send(clientfd, request.c_str(),strlen(request.c_str())+1,0);//将json字符串发送出去
                if(len==-1)
                {
                    cerr<<"send reg msg error"<<request<<endl;
                }
                sem_wait(&rwsem);//等待信号量
            }
            break;
            case 3://quit业务--即结束整个客户端进程
                close(clientfd);
                sem_destroy(&rwsem);//释放信号量资源
                exit(0);
            default:
                cerr<<"invalid input!"<<endl;
                break;
        }
    }

    
}

//显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout<<"========================login user=========================="<<endl;
    cout<<"currebt login user => id:"<<g_currentUser.getId()<<" name:"<<g_currentUser.getName()<<endl;
    cout<<"-----------------------friend list--------------------------"<<endl;
    if(!g_currentUserFriendList.empty())
    {
        for(User &user:g_currentUserFriendList)
        {
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"----------------------group list----------------------------"<<endl;
    if(!g_currentUserGroupList.empty())
    {
        for(Group &group:g_currentUserGroupList)
        {
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user:group.getUsers())
            {
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
            }
        }
    }
    cout<<"============================================================="<<endl;
}

//获取系统时间--用于聊天时的时间信息
string getCurrentTime()
{
    auto tt=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm*ptm = localtime(&tt);
    char data[60] = {0};
    sprintf(data,"%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year+1900, (int)ptm->tm_mon+1,(int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(data);
}

//处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    
    if(0!=responsejs["errno"].get<int>())//登录失败
    {
        cerr<<responsejs["errmsg"]<<endl;
        g_isLoginSuccess = false;//登录失败，置为false
    }
    else  //登录成功
    {
        //记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        //记录当前用户的好友列表信息
        if(responsejs.contains("friends"))
        {
            //初始化操作  防止重新登录后重复添加相同的信息
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for(string &str : vec)
            {
                json js = json::parse(str);
                User user;
                
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
        //记录当前用户的群组列表信息
        if(responsejs.contains("groups"))
        {
            //初始化操作  防止重新登录后重复添加相同的信息
            g_currentUserGroupList.clear();

            vector<string> vec1 = responsejs["groups"];
            for(string &groupstr:vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for(string &userstr: vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }
        //显示登录用户的基本信息
        showCurrentUserData();

        //显示当前用户的离线消息  有个人消息  也有群消息
        if(responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for(string &str: vec)
            {
                json js = json::parse(str);
                if(ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout<<js["time"].get<string>() << " ["<<js["id"]<<" ]"<<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
                }
                else
                {
                    cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>() << " ["<<js["id"]<<" ]"<<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
                }
            }
        }

        g_isLoginSuccess = true;//登录成功，置为true

    }
}
void doRegResponse(json &responsejs);
//处理注册响应逻辑
void doRegResponse(json &responsejs)
{
    
    if(0!=responsejs["errno"].get<int>())//注册失败
    {
        cerr<<"name is already exist, register error!"<<endl;
    }
    else//注册成功
    {
        cout<<"register success, userid is "<<responsejs["id"]<<", do not forget it!"<<endl;
    }
    //不过注册和登录不一样，注册成功就是注册成功了，不需要做额外的东西，完成后继续回到菜单页面，但是登录成功则需要转去聊天命令页面循环，所以需要额外变量去判断是否成功，要不要去转
}
//子线程专门用作接受线程   为了避免错误，主线程就不要做任何接受消息的逻辑处理了。
void readTaskHandler(int clientfd)
{
    for(;;)
    {
        char buffer[1024]={0};
        
        int len = recv(clientfd,buffer,1024,0);
        if(-1==len || 0==len)
        {
            close(clientfd);
            exit(-1);
        }

        //接受服务器转发的数据，反序列化为json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(ONE_CHAT_MSG == msgtype)
        {
            cout<<js["time"].get<string>() << " ["<<js["id"]<<" ]"<<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(GROUP_CHAT_MSG == msgtype)
        {
            cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>() << " ["<<js["id"]<<" ]"<<js["name"].get<string>()<<" said: "<<js["msg"].get<string>()<<endl;
            continue;
        }

        //登录的响应消息也放到子线程判断
        if(LOGGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js);//处理登录响应的业务逻辑
            //处理完了后，不管是登录成功还是失败，通知主线程 登录结果处理完成
            sem_post(&rwsem);
            continue;
        }

        //注册的响应消息
        if(REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);  //注册响应消息处理完成，通知主线程
            continue;
        }
    }
}


void help(int fd=0,string="");
void chat(int,string);
void addfriend(int,string);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);

unordered_map<string,string> commandMap={
    {"help","显示所有支持的命令, 格式help"},
    {"chat","一对一聊天, 格式chat:friendid:message"},
    {"addfriend","添加好友, 格式addfriend:friendid"},
    {"createfroup","创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组, 格式addgroup:groupid"},
    {"groupchat","群聊, 格式groupchat::groupid:message"},
    {"loginout","注销, 格式loginout"}
};

unordered_map<string, std::function<void(int, string)>> commandHandlerMap =
{
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout} 
};

//主页面聊天程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");
        if(-1==idx)
        {
            command = commandbuf; //比如loginout和help是没有:的
        }
        else
        {
            command = commandbuf.substr(0,idx); //取:之前的字符串作为命令
        }

        auto it = commandHandlerMap.find(command);//查找输入命令是否是系统命令之一

        if(it == commandHandlerMap.end())
        {
            cerr<<"invalid input command!"<<endl;//输入的命令是无效  
            continue;
        }

        //调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不要修改改函数。
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));//调用命令处理方法  第一个是套接字，第二个是剩下的字符串，不包含命令后的:

    }

}


//把系统至此的命令和对应注释都打印一遍
void help(int,string)
{
    cout<<"show command list >>> "<<endl;
    for(auto &p: commandMap)
    {
        cout<<p.first<<" : "<<p.second<<endl;
    }
    cout<<endl;
}


void chat(int clientfd,string str)
{
    int idx = str.find(":");  //剩下friendid:message
    if(-1 == idx)
    {
        cerr<<"chat command invalid!"<<endl;
        return ;
    }

    int friendid = atoi(str.substr(0,idx).c_str());  //冒号前面的划分为id
    string message = str.substr(idx+1,str.size()-idx);  //冒号后面的划分为msg

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"send chat msg error ->"<<buffer<<endl;
    }

}


void addfriend(int clientfd,string str)
{
    int friendid = atoi(str.c_str()); //将字符串转换成整数
    json js;
    js["msgid"] = ADD_FRIEDN_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len)
    {
        cerr<<"send addfriend msg error ->"<<buffer<<endl;
    }

}


//创建群组业务
void creategroup(int clientfd,string str)
{   
    int idx = str.find(":");
    if(-1==idx)
    {
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }
    string groupname = str.substr(0,idx);
    string groupdesc = str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len)
    {
        cerr<<"send creatgroup msg error ->"<<buffer<<endl;
    }

}
void addgroup(int clientfd,string str)
{
    int groupid = atoi(str.c_str()); //将字符串转换成整数
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len)
    {
        cerr<<"send addgroup msg error ->"<<buffer<<endl;
    }

}

//groupchat::groupid:message
void groupchat(int clientfd,string str)
{
    int idx = str.find(":");
    if(-1==idx)
    {
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }
    int groupid= atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len)
    {
        cerr<<"send groupchat msg error ->"<<buffer<<endl;
    }
}
void loginout(int clientfd,string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len)
    {
        cerr<<"send loginout msg error ->"<<buffer<<endl;
    }
    else
    {
        isMainMenuRunning = false; //退出主菜单聊天页面   返回初始登录页面
    }
}