#include "db.h"
#include <muduo/base/Logging.h>

static string server = "192.168.0.101"; //数据库服务器ip地址
static string user = "root";   //数据库登录用户名
static string password = "123456";  //数据库登录密码
static string dbname = "chat";    //要连接的数据库名称

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr); // 初始化一个MYSQL对象
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}

// 连接数据库
bool MySQL::connect()
{
    //建立实际连接
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        // c和c++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文会显示为乱码
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
        fprintf(stderr, "Failed to connect to database: Error: %s\n",
          mysql_error(_conn));
    }
    return p;
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败";
        return false;
    }
    return true;
}

// 查询操作
MYSQL_RES * MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败";
        return nullptr;
    }
    return mysql_use_result(_conn);
}


//获取连接
MYSQL * MySQL::getConnection()
{
    return _conn;
}