#include "EventLoop.h"
#include "logger.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "Poller.h"
#include "Channel.h"

// 防止一个线程创建多个EventLoop  thread_local
__thread EventLoop *t_loopInThisThread = nullptr;
// 定义默认的Poller IO复用接口的 超时时间
const int kPollTimeMs = 10000;
// 创建wakeupfd 用来 notify唤醒 subReactor 处理新来的 channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error: %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_)), currentActiveChannel_(nullptr)
{
    LOG_DEBUG("Eventloop created %p in thread %d \n ", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another Eventloop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将要监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}
// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);
    while (!quit_)
    {
        activeChannels_.clear();
        // 监听的两类fd 一种是client fd  一种是 wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些Channel发生事件了然后上报给EventLoop 通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        // IO线程 mainLoop 做的是 accept的工作（接受新用户连接） 用channel打包fd  已连接用户的channel得分发给subloop
        // mainloop 事先注册一个回调cb需要 subloop执行  wakeup subloop以后执行下面的方法 执行之前的mainloop注册的cb操作
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping. \n", this);
     looping_ = false;
}
// 退出事件循环
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread()) // 如果是在其他线程 调用的quit 在一个subloop(worker)中 调用了 mainloop（IO）的quit
    {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // loop线程中 执行cb
    {
        cb();
    }
    else // 在非当前loop线程中执行cb 就需要唤醒loop所在线程执行cb
    {
        /**/
        queueInLoop(cb);
    }
}
// 把cb放入队列中 唤醒loop所在的线程 执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的需要执行上面回调操作的loop线程
    if (!isInLoopThread() || callingPendingFunctors_) // 当前loop正在执行回调，但是loop又有了新的回调，
    {

        wakeup(); // 唤醒loop所在线程
    }
}
//用来唤醒loop所在的线程 像wakeupfd写一个数据 wakeupChannel就发生读事件 当前loop就会被唤醒
void  EventLoop::wakeup()
{
    uint64_t one =1;
    size_t n=write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup()writes %lu bytes instead of 8\n",n);
    }
}
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}
void EventLoop::doPendingFunctors()
{
   std::vector<Functor>functors;
   callingPendingFunctors_=true;
   {
    std::unique_lock<std::mutex>lock(mutex_);
    functors.swap(pendingFunctors_);
   }
   for(const Functor&functor:functors)
   {
    functor(); //执行当前loop需要执行的回调操作
   }
   callingPendingFunctors_=false;
}