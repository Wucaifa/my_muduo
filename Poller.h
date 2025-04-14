#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO复用模块
class Poller : Noncopyable { 
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // 给所有的IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    // 检查channel是否在poller中
    virtual bool hasChannel(Channel* channel) const;

    // EventLoop通过该接口获取默认的IO复用的具体实现
    // 最好别在这个类里实现（需要包含派生类），不好，在一个文件中单独实现
    static Poller* newDefaultPoller(EventLoop* loop);

protected:
    // key:socketfd; value:socketfd所属的Channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;  // fd -> channel

private:
    EventLoop* ownerLoop_;  // poller所属的EventLoop
};