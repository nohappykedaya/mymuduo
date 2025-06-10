#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

/*
nocopyable 禁止拷贝构造和赋值的基类 防止资源拷贝
std::function<void (eventloop*)> 回调函数类型 函数参数为 eventloop* 

*/

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseloop, const std::string &nameArg)
    : baseLoop_(baseloop), name_(nameArg), started_(false), numThreads_(0), next_(0)
{
}
EventLoopThreadPool::~EventLoopThreadPool()
{
}
void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    started_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);//生成线程名
    //每个EventLoopThread创建调用startloop会新建一个线程并运行loop-》loop 并返回eventloop*
    //loops_保存所有工作线程的Eventloop*
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop()); // 底层创建线程 绑定一个新的EventLOOP 并返回 该loop、地址
    }
    // 整个服务端只有一个线程运行着 baseloop
    if (numThreads_ == 0 && cb) //当线程池为0时只使用主线程的Eventloop运行逻辑 
    {
        cb(baseLoop_);
    }
}
// 如果工作在多线程中 baseloop会默认以轮询的方式 分配channel给subloop
EventLoop *EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;
    if (!loops_.empty())//通过轮询获取下一个处理事件的loop
    {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}
//获取当前线程池中所有可用的eventloop*  用  如果线程池为空返回主线程的loop  否则返回loops_
std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}