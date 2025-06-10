#pragma once

#include "nocopyable.h"
#include <functional>
#include<vector>
#include <memory>
#include<string>

class EventLoop;
class EventLoopThread;

//对多个工作线程的封装 每个线程内部跑一个 EventLoop 用于处理channel事件 实现网络事件的多线程并发处理
//EventLoopThreadPool会预先创建多个EvenetLoopThread启动线程把EventLoop* 缓存在loops_中 主线程在接受新连接时可以轮询选择一个 subloop来处理它

class EventLoopThreadPool:nocopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseloop,const std::string &nameArg);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads){numThreads_=numThreads;}
    void start(const ThreadInitCallback &cb=ThreadInitCallback());
    //如果工作在多线程中 baseloop会默认以轮询的方式 分配channel给subloop
    EventLoop *getNextLoop();
    std::vector<EventLoop*>getAllLoops();
    bool started() const {return started_;}
    const std::string name() const {return name_;}
private:
    EventLoop *baseLoop_;//Eventloop loop; 主线程的事件循环
    std::string name_;//线程池的名字
    bool started_;//是否已经启动线程池
    int numThreads_;//线程数量
    int next_;//当前轮询的下标
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//拥有多个EventLoopThread对象的智能指针容器
    std::vector<EventLoop*>loops_;//保存每个EventLoopThread对应的eventloop*

};