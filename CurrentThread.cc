#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread //把tid缓存相关的变量和函数都放在这儿命名空间里避免名字冲突
{
    __thread int t_cachedTid =0; //__thread是c++中一个线程局部变量修饰符 每个线程都有自己单独的一份变量副本，互不影响就是说t_cachedTid是每个线程私有的int变量 
    void cacheTid()
    {
        if(t_cachedTid==0)
        {
            //通过linux系统调用 获取当前线程的pid值
            t_cachedTid =static_cast<pid_t>(::syscall(SYS_gettid));
        }//(::syscall(SYS_gettid))是linux的系统调用 获取当前线程的真实tid static_cast<pid_t>保证了类型安全
    }
}