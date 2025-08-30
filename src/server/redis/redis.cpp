#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
:_publish_context(nullptr),_subscribe_context(nullptr)
{

}
Redis::~Redis()
{
    if(_publish_context!=nullptr)
    {
        redisFree(_publish_context);
    }

    if(_subscribe_context!=nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 链接redis服务器
bool Redis::connect()
{
    //负责publish的上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);
    if(_publish_context == nullptr)
    {
        cerr<<"connect redis failed"<<endl;
        return false;
    }

    _subscribe_context =  redisConnect("127.0.0.1",6379);
    if(_subscribe_context == nullptr)
    {
        cerr<<"connect redis failed"<<endl;
        return false;
    }

    //在独立的线程中，监听通道上的事件，有消息就给业务层进行上报
    thread t([&]() {
        observer_channel_message();
    });

    t.detach();

    cout<<"connect redis-server success!"<<endl;
    return true;

}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply* reply = (redisReply*) redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str());  //PUBLISH命令一执行就会有结果回复，因此不会堵塞
    if(nullptr == reply)
    {
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    //"SUBSCRBE"命令会造成线程堵塞等到直到通道里有消息（回复），即便收到了消息，又会再次堵塞等待，这里只做订阅消息，不接收通道消息
    //通道消息的接收专门由observer_channel_message函数所在的独立线程执行，这样就不会阻塞主线程了
    //redisCommand函数会先调用redisAppendCommand将"SUBSCRIBE %d"缓存到本地，然后调用redisBufferWrite将命令发布到redis服务器上,
    //并最后调用redisGetReply来以堵塞的方式等待结果
    //显然，我们只希望订阅一下就完事了，然后该干嘛干嘛，主线程不应该在这里等着，所以不能直接使用redisCommand函数做这件事，而应该拆开来
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel))
    {
        cerr<<"subscribe command failed!"<<endl;
        return false;
    }

    int done=0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done))  
        {
            cerr<<"subscribe command failed!"<<endl;
            return false;
        }
    }
    //这这样就只是执行订阅命令而已，不需要在这里阻塞等待响应
    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    //同上
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d",channel))
    {
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }

    int done=0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context,&done))  
        {
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }
    //这这样就只是执行取消订阅命令而已，不需要在这里阻塞等待响应
    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply * reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subscribe_context,(void**)&reply))  //从订阅上下文_subscribe_context里  直接调用redisGetReply来堵塞等待读取通道里的消息了
    {
        //订阅收到的消息是一个带三元素的数组 所订阅的通道如果来消息会是三个数据存在element里下标分别为0 1 2：  消息头"message"   通道号"13"  具体消息内容"hello"
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str !=nullptr)
        {
            //如果有消息不为空，则给业务层上报当前通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr<<">>>>>>>>>>>>>>>>>  observer_channel_message quit  <<<<<<<<<<<<<<<<<<<<"<<endl;
    
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}