#pragma once
#include "nocopyable.h"
#include <memory>
#include <atomic>
#include <string>

#include "Buffer.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

/*
Tcpserver通过 Acceptor 有一个新用户连接通过accept函数拿到connfd    打包 tcpconnection  设置回调  设置给 channel
注册到poller上 监听到事件就是 有channel的监听操作

*/

class Channel;
class EventLoop;
class Socket;

class TcpConnection : nocopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                  const InetAddress &localAddr_,
                  const InetAddress peerAddr_);
    ~TcpConnection();
    EventLoop *getloop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }
    // 连接建立
    void connectEstablished();
    // 连接销毁
    void connectDestroyed();
    void shutdown();
    void send(const std::string &buf);
    void sendInloop(const void *message, size_t len);

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    void setSate(StateE state) { state_ = state; }
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void shutdownInloop();
    EventLoop *loop_; // 这里绝对不是 baseloop 因为Tcpconnection都是在subloop里面管理的
    const std::string name_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    std::atomic_int state_;
    bool reading_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后得回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    size_t highWaterMark_;

    Buffer inputBuffer_;  // 接受数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
};