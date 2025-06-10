#pragma once

#include "nocopyable.h"
#include <functional>
#include "Timestamp.h"
#include <memory>
class EventLoop;

// Channel理解为,封装了sockfd 套接字文件描述符 和其感兴趣的event，如 EPOOLIN EPOLLOUT事件
// 还绑定了poller返回的具体事件
//理清楚 EventLoop  Channel  poler的关系在模型上对应Demutiplex
class Channel : nocopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd); 
    ~Channel();
    // fd得到poller通知以后 处理事件的
    void handleEvent(Timestamp receiveTime);//

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ =std::move(cb);}
    void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}
    void setErrorCallback(EventCallback cb){errorCallback_=std::move(cb);}
    //防止当channel被手动remove掉 channel还在执行回调操作
    void tie(const  std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const{return events_;}
    int set_revents(int revt){return revents_=revt;}
   
    //设置fd相应的事件状态
    void enableReading(){events_|=kReadEvent; update();}//关注EPOLLIN
    void disableReading(){events_&= ~kReadEvent;update();} //取消关注EPOLLIN
    void enableWriting(){events_|=kWriteEvent; update();} //关注EPOLLOUT
    void disableWreting(){events_&= ~kWriteEvent;update();}//
    void disableAll(){events_=kNoneEvent;update();}

    //返回fd当前的事件状态
    bool isNoneEvent() const{return events_==kNoneEvent;}
    bool isWriting()  const {return events_ &kWriteEvent;}
    bool isReading() const {return events_ &kReadEvent;}

    int index(){return index_;}
    void set_index(int idx){index_=idx;}

    //one loop per
    EventLoop*ownerLoop(){return loop_;}
    void remove();
private:
    void update();
    void handleEventWithGuard(Timestamp reveiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环   / loop_通过loop_将事件通知到事件循环，调用updateChannel等方法更新事件监听
    const int fd_;    // fd，poller监听的对象  /fd是文件描述符 标识需要监听的事件资源
    int events_;      // 注册fd感兴趣的事件   告诉poller需要监听哪些事件
    int revents_;     // poller返回的具体发生的事件 系统返回的事件
    int index_;       // 状态标识 

    std::weak_ptr<void> tie_; // 延长对象生命周期 防止对象在处理事件时被提前析构 
    bool tied_;

    // 因为Channel通道里面能够获知fd最终发生的具体的事件revents所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};