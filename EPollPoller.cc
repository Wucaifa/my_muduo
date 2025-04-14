#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <error.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>

const int kNew = -1;        // 未添加，channel的成员变量index_的初始值
const int kAdded = 1;       // 已添加
const int kDeleted = 2;     // 已删除

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    // 实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func = %s -> fd total counts : %lu\n", __FUNCTION__, channels_.size());
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    // 这里将errno保存下来，避免epoll_wait被信号中断
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0) {
        LOG_INFO("%d events happened\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == static_cast<int>(events_.size())) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    } else {
        if (savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("EPollPoller::poll() error: %d \n", savedErrno);
        }
    }
    return now;
}

// channel updata remove -> EventLoop updateChannel removeChannel -> Poller updateChannel removeChannel
/**
 *              EventLoop
 *             /    \
 *    channellist  poller
 *                /      \ 
 *           channelmap 
 * channellist大小>=channelmap大小
 */
void EPollPoller::updateChannel(Channel* channel) {
    const int index = channel->index();
    LOG_INFO("func = %s fd = %d events = %d index = %d\n", __FUNCTION__, channel->fd(), channel->events(), index);
    if (index == kNew || index == kDeleted) {
        int fd = channel->fd();
        if (index == kNew) {
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {    // 已经注册过
        // 更新channel
        int fd = channel->fd();
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中的channelmap中删除channel
void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    LOG_INFO("func = %s fd = %d events = %d index = %d\n", __FUNCTION__, channel->fd(), channel->events(), channel->index());
    channels_.erase(fd);
    int index = channel->index();
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// 从epoll中添加、修改、删除fd的事件
void EPollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();

    event.data.fd = fd;
    event.data.ptr = channel;
    event.events = channel->events();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if(operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod error: %d\n", errno);
        }
    }
}

