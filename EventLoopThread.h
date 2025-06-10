#pragma once 

#include "nocopyable.h"
#include <functional>
#include "Thread.h"
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;
//主要职责： 在一个新线程创建并运行一个eventloop 提供startloop接口返回这个eventloop* 支持
//线程初始化回调函数  
class EventLoopThread :nocopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb=ThreadInitCallback()
,const std::string & name =std::string());
    ~EventLoopThread();

    EventLoop * startLoop();

private:
    void threadFunc();

    EventLoop * loop_;//指向新线程中创建的eventloop对象
    bool exiting_;//标志是否退出 
    Thread thread_;// 封装线程的对象 
    std::mutex mutex_;//互斥锁
    std::condition_variable cond_;//条件变量 用于主线程等待子线程创建好eventloop 
    ThreadInitCallback callback_;   //线程初始化用户自定义回调
};