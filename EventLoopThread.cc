#include "EventLoopThread.h"
#include "EventLoop.h"

//将成员函数threadfunc绑定到当前对象this 用于创建线程
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
:loop_(nullptr)
,exiting_(false)
,thread_(std::bind(&EventLoopThread::threadFunc,this),name)
,mutex_()
,cond_()
,callback_(cb)                                                                                                          
{

}
EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if(loop_!=nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start();//启动底层的线程
    EventLoop *loop =nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
    while( loop_==nullptr)
    {
     cond_.wait(lock);
    }
    loop=loop_;
    }
return loop;
}
//这个方法是在单独的新线程里面运行的
void EventLoopThread::threadFunc()
{

EventLoop loop;//创建一个独立的eventloop  和上面线程是一一对应的   one loop per thread
if(callback_)
{
    callback_(&loop);//调用初始化回调
}

{
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=&loop;
    cond_.notify_one();//唤醒主线程
}
    loop.loop();//eventloop loop => poler.poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_=nullptr;
}
/*
语法：
   cond_.wait(lock) 等待通知 自动释放并且重新获取锁  
   cond_.notify_one() 唤醒一个等待线程
*/