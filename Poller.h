#pragma once

#include "nocopyable.h"
#include <vector>
#include <unordered_map>
#include "Timestamp.h"

class Channel;
class EventLoop;

// muduo 库中 多路事件分发器的核心IO复用模块
class Poller:nocopyable
{
public:
    using ChannelList =std::vector<Channel*>;
     
    Poller(EventLoop *loop);
    virtual ~Poller() =default;
    //给所以IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs,ChannelList *activeChannels)=0;//muduo最终会通过它调用epoll_wait()或者别的实现等待事件发生
    virtual void updateChannel(Channel*channel)=0;//让poller注册修改删除某个fd事件（本质会调用epoll_ctl）
    virtual void removeChannel(Channel *channel)=0;//让poller把某个不在需要监听的fd移除
    //判断参数Channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const; // 判断某个channel是否被监听

    //EventLoop 可以通过该接口获取默认的IO复用的具体实现
    static Poller * newDefaultPoller(EventLoop * loop);
private:
     EventLoop*ownerLoop_;//定义Poller所属的事件循环 EventLoop   
protected:
    //map的key：sockfd value:sockfd所属的Channel通道类型
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;
};