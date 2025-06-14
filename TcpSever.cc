#include "TcpSever.h"
#include "logger.h"
#include <strings.h>
#include <functional>
#include "TcpConnection.h"


static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpSever::TcpSever(EventLoop *loop, 
                    const InetAddress &listenAddr,
                   const std::string &nameArg,
                   Option option)
    : loop_(CheckLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()), name_(nameArg), acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), threadpool_(new EventLoopThreadPool(loop, name_)), connectionCallback_(), nextConnId_(1),started_(0)
{
    // 当有新用户连接时会执行TcpSever::newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpSever::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}
TcpSever::~TcpSever()
{
    for(auto &item : connections_)
    {
        //这个局部的shared_ptr对象 出有括号 可以自动释放new出来的 tcpconnection对象
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getloop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn)
        );
    }
}
//有一个新的客户端连接 acceptor会执行这个回调操作   
void TcpSever::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop * ioLoop = threadpool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf , " -%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpSever::newConnection [ %s]- new connection [%s] from %s \n",name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
    //通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0)
    {
        LOG_ERROR("sockets::getLocalAddr");
        
    }
    InetAddress localAddr(local);

    //根据连接成功的sockfd 创建tcpConnection连接到对象
    TcpConnectionPtr conn( new TcpConnection(
        ioLoop,
        connName,
        sockfd,
        localAddr,
        peerAddr
    ));
    connections_[connName] = conn;
    //下面的回调都是 用户设置给Tcpserver =》tcpconnection =》channel-》poller-》 notifychannel 调用回调

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(
        std::bind(&TcpSever::removeConnection,this,std::placeholders::_1)
    );
    //直接调用TcpConnection::connectEstablished 
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
}
void TcpSever::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpSever::removeConnecitonInLoop,this,conn)
    );
}
void TcpSever::removeConnecitonInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO ("TcpSever::removeConnecitonInLoop [%s] - connection %s \n",name_.c_str(),conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getloop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed,conn)
    );

}
// 设置底层subloop的个数
void TcpSever::setThreadNum(int numThreads)
{
    threadpool_->setThreadNum(numThreads);
}
// 开启服务器监听
void TcpSever::start()
{
    if(started_++ == 0) //防止一个TcpSerever对象被start多次
    {
        threadpool_->start(threadInitCallback_);//启动底层的loop线程池
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));  
    }
}