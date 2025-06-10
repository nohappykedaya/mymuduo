#include "Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"
#include "logger.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel() {}

//channel的tie方法什么时候调用过？
//
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/*
    当改变Channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
    EventLoop =>Channellist  Poller
*/
void Channel::update()
{
    // 通过Channel所属的eventloop 调用poller的相应的方法 注册fd的events事件
    // addcode....
    loop_->updateChannel(this);
}

// 在Channel所属的eventLOOP中 把当前的Channel删除掉
void Channel::remove()
{
    // add...
     loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{//当epoll_wait返回某个事件时EventLoop会调用channel的这个函数让他执行对应的回调
    if(tied_)
    {//如果绑定了shared_ptr 就尝试延长生命周期 避免野指针
        std::shared_ptr<void>guard=tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

//根据poller监听的channel发生的具体事件  由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp reveiveTime)
{//根据epoll返回的revents_一个个去判断发生了什么事然后调用对应的回调
    LOG_INFO("channel handleEvenet revents: %d",revents_);
    if((revents_ & EPOLLHUP)&&!(revents_ & EPOLLIN))
    {//EPOLLIN —— 可读事件  EPOLLOUT —— 可写事件EPOLLPRI —— 有“紧急数据”  EPOLLERR —— 出错了  EPOLLHUP —— 对方关闭了连接
 
        if(closeCallback_)
        {
            closeCallback_();
        }
    }
    if(revents_ &EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(reveiveTime);
        }
    }
    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }
}