#include "InetAddress.h"
#include <strings.h>
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip )
{
     bzero(&addr_,sizeof addr_);//将addr_的结构体内存清零 消除未初始化的随机值干扰
     addr_.sin_family = AF_INET; //设置地址族为IPV4 告诉 系统该地址结构用于IPV4协议
     addr_.sin_port=htons(port); //设置端口号 将主机字节序转换成网络字节序
     addr_.sin_addr.s_addr=inet_addr(ip.c_str());//将点分十进制IP字符串转换为32位网络字节序的二进制值

}

std::string InetAddress::toIp() const
{
    //addr_
    char buf[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);//将二进制格式的IPv4地址转换为人类可读的字符串（点分十进制格式）
    return buf;
}
std::string InetAddress::toIpPort() const //
{
    // ip ： port
    char buf[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf); //将二进制的ipv4地址转换为字符串格式
    size_t end=strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return  ntohs(addr_.sin_port);
}

// #include<iostream>

// int main()
// {
//     InetAddress addr(8080);
//     std::cout <<addr.toIpPort() << std::endl;
// }