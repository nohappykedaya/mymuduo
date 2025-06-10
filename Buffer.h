#pragma once

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }
    size_t writeableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    size_t prependableByte() const
    {
        return readerIndex_;
    }
    // 返回缓冲区中可读取数据的起始地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }
    // onMessage string -> buffer
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {

            readerIndex_ += len; // 说明应用只读取了  就是len
        }
        else
        {
            retieveAll();
        }
    }
    void retieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }
    // 把onmessage函数上报的buffer数据转成string类型的数据
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据  已经读取出来  进行复位操作
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writeableBytes() < len)
        {
            makespace(len); //
        }
    }
    //把data，data+len 内存上的数据 添加到writeable 缓冲区中
void append(const char * data,size_t len)
{
    ensureWriteableBytes(len);
    std::copy(data,data+len,beginwrite());
    writerIndex_+=len;
}
char * beginwrite()
{
    return begin()+writerIndex_;
}
const char * beginwrite() const 
{
     return begin()+writerIndex_;
}
//从fd上读取数据
size_t readFd(int fd , int *saveErrno);
ssize_t writeFd(int fd, int* saveErrno);
private:
    char *begin()
    {
        // it.operator*()
        return &*buffer_.begin(); // vector底层数组首元素的地址 也就是数组的起始地址
    }
    const char *begin() const
    {
        return &*buffer_.begin();
    }
    void makespace(size_t len)
    {
        if (writeableBytes() + prependableByte() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_, len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
