#include "CurrentThread.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>

namespace CurrentThread
{
    __thread int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = static_cast<int>(syscall(SYS_gettid));
        }
    }
}  // namespace CurrentThread