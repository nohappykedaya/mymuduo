#pragma once 
#include "nocopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>

/*监听客户端的连接请求 并在有新的连接时 通过回调函数 将连接交给上层处理 （tcpserver）*/

class EventLoop;
class InetAddress;
// 封装了listenfd（监听套接字）的创建 绑定 监听 将listenfd包装成channel 注册到eventloop中
//在有连接到达时 执行回调通知上层处理
class Acceptor
{
public:
    using NewConnectionCallback=std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop *loop,const InetAddress &listenAddr ,bool reuseport);
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        NewConnectionCallback_=cb;
    }
    bool listenning()const{return listenning_;}
    void listen();
private:
    void headleRead();
    EventLoop * loop_;//Acceptor 用的就是 用户定义的那个baseLoop也称作mainLoop
    Socket acceptSocket_;//封装的listenfd
    Channel acceptChannel_;//封装listenfd事件 
    NewConnectionCallback NewConnectionCallback_;// 新连接到达时的回调
    bool listenning_;// 是否已经在监听
};