#include "EPollPoller.h"
#include "logger.h"
#include <errno.h>
#include "Channel.h"
#include <string.h>
#include <unistd.h>

//channel 未添加到poller中
const int kNew = -1; //channel的成员 index_ = -1 
//channel 已添加到poller中 
const int kAdded = 1;
//channel从poller中删除 
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop) : Poller(loop), 
                epollfd_(::epoll_create1(EPOLL_CLOEXEC)), 
                events_(kInitEventListSize)
{//调用epoll_create1(EPOLL_CLOEXEC)创建epoll实例 初始化events_为默认大小 如果失败则打印fatal错误
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error: %d\n", errno);
    }
}
EPollPoller::~EPollPoller()
{
    ::close(epollfd_); //销毁时关闭 epollfd_
}

Timestamp  EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    //实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s=>fd total count:%lu\n",__FUNCTION__,channels_.size());

   //调用epoll_wait阻塞等待事件发生 若返回值numEvents >0 就填充活跃通道列表 activechannels 若返回值为events_.size()表示数组满了进行扩容 
    int numEvents =::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno=errno;
    Timestamp now(Timestamp::now());

    if(numEvents>0)
    {
        LOG_INFO("%d events happened \n",numEvents);
        fillActiveChannels(numEvents,activeChannels);//解析epoll_event数组 把活跃的channel填入activechannels
        if(numEvents == events_.size())
        {
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else
    {
        if(saveErrno!=EINTR) //表示信号被中断 不是错误
        {
            errno =saveErrno;
            LOG_ERROR("EPollPoller::Poll() err!\n");
        }
    }
    return now;//最后返回当前时间戳 用于计算io阻塞时间
}
//channel  update remove -> eventloop updatechannel removechannel -》 poller updatechannel  removechannel

/*
    EventLoop 包含了 channellist   poller    
    poller中有 channelMap  <fd ,channel*>

*/

void EPollPoller::updateChannel(Channel *channel)
{
    //
    const int  index = channel->index();
    LOG_INFO("func=%s=>fd = %d events = %d  index=%d\n",__FUNCTION__,channel->fd(),channel->events(),index);

    if(index ==kNew || index ==kDeleted)
    {//新channel加入channels_映射然后使用epoll_ctl_add注册
        if(index == kNew)
        {
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);

    }
    else //channel已经在poller上注册过了
    {//若该channel现在不关心任何时间 使用EPOLL_CTL_DEL删除 否则 mod更新事件
        int fd=channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }
}
//从poller中删除channel
void  EPollPoller::removeChannel(Channel *channel)
{//从 channels_ map上移除 如果还注册在epoll上 就调用 del 把状态设置回kNew 表示干净未注册状态
    int fd=channel->fd();
    channels_.erase(fd);
    LOG_INFO("func=%s=>fd = %d\n",__FUNCTION__,fd);

    int index=channel->index();
    if(index==kAdded)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}
// 填写活跃的连接
void  EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const 
{
    for(int i=0;i<numEvents;++i)
    {
    //epoll中的event.data.ptr存的是channel*指针  遍历返回的epoll_event数组把每个触发的channel设置好revents 放入activechannels列表提供给eventloop调用处理
        Channel * channel =static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);//EventLoop就拿到了它的poller给它干事的所以发生事件的channel列表

    }
}
// 更新channel 通道 epoll_ctl add /mod / del
void EPollPoller:: update(int operation, Channel *channel) 
{
    epoll_event event;
    bzero(&event,sizeof event);
    int fd=channel->fd();
    event.events=channel->events(); //设置监听的事件
    event.data.fd=fd; 
    event.data.ptr =channel;
//把 Channel* 存进 event.data.ptr，这样 epoll_wait 返回事件后可以找回原来的 Channel

    if(::epoll_ctl(epollfd_,operation,fd,&event)<0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_Ctl del error: %d\n",errno);
        }
        else
        {
            LOG_FATAL("epoll_Ctl add/mod error: %d\n",errno);
        }
    }
}