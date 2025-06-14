#pragma once

#include "nocopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include<string>
#include<atomic>

class Thread : nocopyable
{
public:
    using ThreadeFunc = std::function<void()>;
    explicit Thread(ThreadeFunc func,const std::string &name=std::string());
    ~Thread();
    void start();
    void join();

    bool started() const{return started_;}
    pid_t tid() const {return tid_;}
    const std::string&name()const{return name_;}
    static int numCreated(){return numCreated_;}
private:
    void setDeafuatName();
    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadeFunc func_;
    std::string name_;
   static std::atomic_int numCreated_;
};