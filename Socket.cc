#include "Socket.h"
#include "logger.h"
#include "InetAddress.h"

#include<iostream>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include  <netinet/tcp.h>
#include <sys/socket.h>


Socket::~Socket()
{
    close(sockfd_);
}
//将socket绑定到指定地址
void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0!=::bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail\n",sockfd_);
    }
}
//监听连接
void Socket::listen()
{
    if(0!=::listen(sockfd_,1024))
    {
        LOG_FATAL("listen sockfd:%d fail\n",sockfd_);
    }
}
//接受连接  接受客户端连接请求 返回新的连接文件描述符 connfd
int Socket::accept(InetAddress *peeraddr)
{
    /*
    1.accept参数不合法
    2 对返回的connfd没有设置非阻塞
        Reactor模型 one loop thread   
        poller + non-blocking  IO
    */
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr,sizeof addr); //清零
    int connfd=::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >=0)
    {
        peeraddr->setSockAddr(addr); //成功后把客户端地址信息写回去
    }
    return connfd;
}
//关闭写端
void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR)<0)
    {
        LOG_ERROR("shutdownWrite error\n");
    }
}

//以下是设置socket选项
//TCP_NODELAY： true是禁用Nagle算法 减少延迟  false是启用Nagle减少包数量
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof (optval));
}
//设置地址重用 解决 address 已经被使用的问题
void Socket::setReuseAddr(bool on)
{
     int optval = on ?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof (optval));
}
//设置端口重用  linux支持多个socket绑定到同一端口
void Socket::setReusePort(bool on)
{
     int optval = on ?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof (optval));
}
//设置 TCP keepalive机制 用于保持连接活跃 防止僵尸连接
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}