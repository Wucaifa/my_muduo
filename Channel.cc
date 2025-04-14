#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false){
}

Channel::~Channel() {
    
}

// 弱智能指针观察者
// 绑定的对象被销毁时，weak_ptr会自动置空
// 什么时候调用？一个TcpConnection新连接建立时，TcpConnection =》Channel 
// void 是一种不具体的类型，表示这个 shared_ptr 可以指向任何类型的对象（类似于泛型或类型擦除）
void Channel::tie(const std::shared_ptr<void>& obj) {
    LOG_DEBUG("Channel::tie() %p", obj.get());
    tie_ = obj;
    tied_ = true;
}

/**
 * 当改变channel所表示的fd的事件时，updata负责在poller中更新fd相应事件的epoll_ctl
 * 
 */
void Channel::update() {
    // 通过channel所属的EventLoop来调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中删除channel
void Channel::remove() {
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的函数
void Channel::handleEvent(Timestamp receiveTime) {
    // 处理fd的事件
    if (tied_) {
        // lock作用是将弱智能指针转换为一个强智能指针，如果原对象已经被销毁（即引用计数为零），lock() 会返回一个空的 std::shared_ptr。
        auto guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revents: %d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}