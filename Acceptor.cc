#include "Acceptor.h"
#include <sys/types.h>
#include <sys/socket.h>
#include "logger.h"
#include "InetAddress.h"

#include <errno.h>
#include<unistd.h>

static int createNonblocking()
{
    //创建非阻塞+close-on-exec 的Tcpsocket 

    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop), acceptSocket_(createNonblocking()), acceptChannel_(loop, acceptSocket_.fd()),listenning_(false)
{
    acceptSocket_.setReuseAddr(true);//设置IP复用
    acceptSocket_.setReusePort(true);//设置端口复用
    acceptSocket_.bindAddress(listenAddr); // bind 本地地址
    // TcpSever::Start() Acceptor.listen  有新用户的连接要执行一个回调
    acceptChannel_.setReadCallback(std::bind(&Acceptor::headleRead, this));//设置acceptChannel_读事件回调位headleread

}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}
// listenfd有事件发生了  就是有新用户连接了
//当有客户端连接时调用
void Acceptor::headleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);//获取客户端连接
    if (connfd >= 0)
    {
        if (NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
       LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
           LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}