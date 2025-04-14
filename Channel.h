#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>

class EventLoop;

/**
 * EventLoop Channel Poller 代表事件分发器
 * Channel 理解为通道，封装了socketfd和其感兴趣的event，如EPOLLIN、EPOLLOUT等
 * 还绑定了poller返回的具体事件
 * 主要作用是调用回调函数
 */

 class Channel {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的函数
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止channel被销毁时，回调函数还在使用
    void tie(const std::shared_ptr<void>& obj);

    // 设置和获取fd
    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    
    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread
    EventLoop* ownerLoop() { return loop_; }
    void remove();  // 从poller中删除fd

private:
    void update();  // 更新poller中的fd事件
    void handleEventWithGuard(Timestamp receiveTime);

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;   // 事件循环
    const int fd_;      // Poller监听的fd
    int events_;        // Poller感兴趣的事件
    int revents_;       // Poller返回的事件
    int index_;         // Channel在Poller中的索引

    // 监听tie_的对象是否被销毁
    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel可以获取fd发生的事件，所以它负责调用fd的回调函数
    ReadEventCallback readCallback_;  // 读事件回调
    EventCallback writeCallback_;     // 写事件回调
    EventCallback closeCallback_;     // 关闭事件回调
    EventCallback errorCallback_;     // 错误事件回调
 };