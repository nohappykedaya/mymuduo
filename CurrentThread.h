#pragma once
#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread
{
      extern __thread int t_cachedTid;
   void cacheTid();

   inline int tid() 
   {//如果t_cachedTid=0 说明当前线程还没获取tid 那就调用cachetid（）来获取tid并且保存 否则直接使用缓存值 速度快
    if(__builtin_expect(t_cachedTid == 0,0))
    {
        cacheTid();
    }
    return t_cachedTid;
   }
}