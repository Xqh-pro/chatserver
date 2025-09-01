# chatserver
基于muduo网络库实现的集群聊天服务器和客户端源码。    
项目分为网络层、业务层、数据层  
网络层使用muduo库作为核心模块，提供高并发网络IO服务。
业务服务层实现具体的业务处理，提供用户注册、登录、注销、一对一聊天、群聊、添加好友、创建群聊、加入群组等服务。多机服务集群拓展时采用nginx实现tcp负载均衡，并使用Redis的发布-订阅机制作为服务器中间件实现跨服务器交互。
数据层采用MySQL数据库对关键用户数据进行落地，存储用户账号、密码、离线消息、群组列表等信息。


编译项：./autobuild.sh 即可自动编译项目    
  &emsp;或者  
    &emsp;&emsp;cd build/  
         &emsp;&emsp;rm -rf *  
         &emsp;&emsp;cmake ..  
         &emsp;&emsp;make       也可实现项目的编译  
运行：  
      &emsp;cd bin/  
      &emsp;./chatserver ip port  //服务器的ip和端口号    
      &emsp;./chatclient ip port  //接入nginx服务器    
      
目录说明：  
      &emsp;bin/  包含了编译生成的可执行文件      
      &emsp;build/  包含了编译生成的中间文件  
      &emsp;include/  包含了源文件依赖的头文件  
      &emsp;src/  包含了服务端和客户端的所有源文件代码  
      &emsp;thirdparty/   包含json.hpp第三方头文件  
      &emsp;autobuild.sh  自动编译脚本  
      &emsp;CMakeLists.txt   cmake配置文件  
      
      
