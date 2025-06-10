#pragma once 
#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

#include "Timestamp.h"

class Channel;
/*
    epoll的使用 
    epoll_create
    epoll_ctl   add/mod/del
    epoll_Wait
*/
class EPollPoller :public Poller
{
public:
    EPollPoller(EventLoop * loop);
    ~EPollPoller() override;

    //重写基类poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList *activeChannels) override;
    void updateChannel(Channel *channel)override;
    void removeChannel(Channel * channel) override;
private:
    static const int kInitEventListSize =16;
    //填写活跃的连接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels) const ; 
    //更新channel 通道
    void update(int operation, Channel *channel);
    
    using EventList = std::vector<epoll_event>;
    int epollfd_;//epoll实例的文件描述符 由epoll_create1（）得到
    EventList events_;//等待返回的就绪事件数组 初始大小是16 动态扩容
};