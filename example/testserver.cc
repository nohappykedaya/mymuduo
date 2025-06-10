#include <mymuduo/TcpSever.h>
#include <string>
#include <mymuduo/logger.h>
#include <functional>
#include <mymuduo/TcpConnection.h>
#include <mymuduo/Buffer.h>

class EchoServer
{
public:
    EchoServer(EventLoop *loop,
               const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name), loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // 设置合适的loop线程数量 loopthread
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }

private:
    // 连接建立
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("conn up : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("conn down : %s", conn->peerAddress().toIpPort().c_str());
        }
    }
    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // 写端 EPOLLHUB = > CLOSE CALLBACK_
    }
    EventLoop *loop_;
    TcpSever server_;
};

int main()
{
   
    LOG_INFO("Starting EchoServer on port 8000");
    EventLoop loop;
    InetAddress addr(8000);
   // printf("正在绑定端口: %d\n", addr.toPort());   // 添加调试输出
    EchoServer server(&loop, addr, "EchoServer-01"); // acceptor non-bolcking listenfd create bind
    server.start();                                  // listen loopthread => acceptCHannel =>mainLoop
    //printf("服务器已启动，等待连接...\n");
    loop.loop(); // 启动mianloop
    return 0;
}