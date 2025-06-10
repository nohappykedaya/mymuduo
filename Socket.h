#pragma once

#include "nocopyable.h"

class InetAddress;
//封装 spcket fd
class Socket
{
public:
//explicit关键字是防止隐式类型转换 假如不小心写了 Socket s=5 这样的代码
    explicit Socket(int sockfd)
    :sockfd_(sockfd)
    {} 

    ~Socket();
    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on); 
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};