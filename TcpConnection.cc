#include "TcpConnection.h"
#include "logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include <functional>
#include "EventLoop.h"

#include <string>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <errno.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress peerAddr)
    : loop_(CheckLoopNotNull(loop)), name_(nameArg), state_(kConnecting), reading_(true), socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)), localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生 channel会回调相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection:: ctor[%s] at fd =%d\n", name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s]at fd =%d state = %d\n", name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if (n > 0)
    {
        // 已建立连接的用户 又可读时间发送了 调用用户传入的回调操作
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("Tcpconnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWreting();
                if (writeCompleteCallback_)
                {
                    // 唤醒loop_对应的thread线程 执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInloop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("Tcpconnection fd = %d is down ,no more writing \n", channel_->fd());
    }
}
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setSate(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);      // 关闭连接的回调
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError NAME:%s - SO_ERROR:%d\n", name_.c_str(), err);
}
/*
    发送数据 应用写得快 而 内核发送数据慢 需要把带发送数据写入缓冲区  而且设置水位回调
*/
void TcpConnection::sendInloop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    // 之前调用过该connection的shutdown  不能再发送了
    if (state_ == kDisconnecting)
    {
        LOG_ERROR("disconnected ,give up writing!");
        return;
    }
    // 表示 channel第一次开始写数据而且缓冲区没有带发送数据
    if (!channel_->isReading() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然再这里数据全部发送完成 就不再给channel设置 epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nw <0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInloop!");
                if (errno == EPIPE || errno == ECONNRESET) // sigpipe reset
                {
                    faultError = true;
                }
            }
        }
    }
    if (!faultError && remaining > 0) // 说明当前这一次write并没有把数据全部发送出去  剩余数据需要保存缓冲区当中 给channel
    // 注册 epollout事件 poller 发现tcp他是的缓冲区有空间 会通知相应的 socket channe_ 调用 writecallback 回调方法
    // 最终也就是调用 handlewrite方法 把发送缓冲区中 的数据全部发送完成
    {
        // 目前发送缓冲区剩余的待发送的长度
        ssize_t oldlen = outputBuffer_.readableBytes();
        if (oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里一定要注册channel的写事件 否则poller不会给channel通知epollout
        }
    }
}
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInloop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInloop, this, buf.c_str(), buf.size()));
        }
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setSate(kConnected);
    channel_->tie(shared_from_this());

    channel_->enableReading(); //向poller注册channel的epollin事件
//新连接建立执行回调
    connectionCallback_(shared_from_this());
}
// 连接销毁
void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected)
    {
        setSate(kDisconnected);
        channel_->disableAll();//把channel的所以感兴趣的事件从poller中del掉
        connectionCallback_(shared_from_this());

    }
    channel_->remove();//把channel 从 poller中删除掉
}
    void TcpConnection::shutdown()
    {
        if(state_ == kConnected)
        {
            setSate(kDisconnecting);
            loop_->runInLoop(
                std::bind(&TcpConnection::shutdownInloop,this)
            );
        }
    }
    void  TcpConnection::shutdownInloop()
    {
        if(!channel_->isWriting()) //说明当前outputbuffer中的数据发送完成
        {
           socket_->shutdownWrite();//关闭写端
            
        }
    }