# chatserver
基于nginx实现tcp负载均衡的muduo的集群聊天服务器和客户端源码。  

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
      &emsp;bin/ 包含了编译生成的可执行文件      
      &emsp;build/ 包含了编译生成的中间文件  
      &emsp;include/ 包含了源文件依赖的头文件  
      &emsp;src/ 包含了服务端和客户端的所有源文件代码  
      &emsp;thirdparty/  包含json.hpp第三方头文件  
      &emsp;autobuild.sh 自动编译脚本  
      &emsp;CMakeLists.txt  cmake配置文件  
      
      
