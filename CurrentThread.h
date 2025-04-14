#pragma once

namespace CurrentThread
{
    // 当前线程的ID
    extern __thread int t_cachedTid;

    // 线程ID
    void cacheTid();
    // 获取当前线程ID
    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}  // namespace CurrentThread