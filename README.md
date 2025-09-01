# chatserver
基于nginx实现tcp负载均衡的muduo的集群聊天服务器和客户端源码。

编译项：./autobuild.sh 即可自动编译项目  
      或者
         cd build/
         rm -rf *
         cmake ..
         make       也可实现项目的编译
运行：
      cd bin/
      ./chatserver ip port  //服务器的ip和端口号
      ./chatclient ip port  //接入nginx服务器
目录说明：
      bin/ 包含了编译生成的可执行文件  
      build/ 包含了编译生成的中间文件
      include/ 包含了源文件依赖的头文件
      src/ 包含了服务端和客户端的所有源文件代码
      thirdparty/  包含json.hpp第三方头文件
      autobuild.sh 自动编译脚本
      CMakeLists.txt  cmake配置文件
      
      
