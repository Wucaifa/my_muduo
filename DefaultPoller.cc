#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

// 通过环境变量来选择使用的epoll还是poll
Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if(::getenv("MUDUO_USE_POLL")) {
        // return new PollPoller(loop);
    }else {
        return new EPollPoller(loop);
    }
}