#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 事件循环类 主要包含了两个模块 Channel和Poller(epoll的抽象)
class EventLoop : Noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行回调函数
    void runInLoop(Functor cb);
    // 把cb放到队列中，唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在的线程
    // wakeupfd代替了生产者消费者模式的队列
    void wakeup();  

    // EventLoop -> Poller
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 判断EventLoop对象是否在自己的线程中
    bool isInLoopThread() const {
        return threadId_ == CurrentThread::tid();
    }

private:
    // 执行回调函数
    void doPendingFunctors();
    // 唤醒事件循环
    void handleRead();
private:
    using ChannelList = std::vector<Channel*>;
    std::atomic<bool> looping_;  // 是否在事件循环中
    std::atomic<bool> quit_;     // 是否退出事件循环
    std::atomic<bool> callingPendingFunctors_;  // 是否在执行回调函数
    const pid_t threadId_;  // 事件循环所属的线程ID
    Timestamp pollReturnTime_;  // poller返回发生事件的channel的时间点
    std::unique_ptr<Poller> poller_;  // epoll的抽象

    int wakeupFd_;  // 当mainLoop获取一个新用户的channel，通过轮询算法选择一个subLoop，通过该成员唤醒subLoop来处理fd
    std::unique_ptr<Channel> wakeupChannel_;  // 唤醒channel

    ChannelList activeChannels_;  // 活跃的channel
    Channel* currentActiveChannel_;  // 当前活跃的channel

    std::vector<Functor> pendingFunctors_;  // 待执行的回调函数
    std::mutex mutex_;  // 互斥锁,用来保护上面vector的线程安全
};