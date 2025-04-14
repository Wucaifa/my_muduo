#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "EPollPoller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

// 防止一个线程创建多个EventLoop对象
__thread EventLoop* t_loopInThisThread = 0; // 线程局部变量，保存当前线程的EventLoop对象

const int kPollTimeMs = 10000; // epoll的超时时间10s

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error : %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),   
      currentActiveChannel_(nullptr) {
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventLoop都将监听wakeupchannel的EPOLLIN读事件
    wakeupChannel_->enableReading(); // 监听wakeupfd的可读事件
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll(); // 取消监听wakeupfd的可读事件
    wakeupChannel_->remove(); // 从poller中删除wakeupfd
    t_loopInThisThread = nullptr;
    ::close(wakeupFd_);
}

// 开启事件循环
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping\n", this);
    while (!quit_) {
        activeChannels_.clear();
        // poller返回发生事件的channel
        // 监听两类fd 一种是wakeupfd，一种是用户注册的channel
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (auto channel : activeChannels_) {
            // EventLoop::handleEvent() -> Channel::handleEvent()
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        // 执行当前EventLoop事件循环需要处理的回调操作
        // mainLoop注册给subLoop的回调函数，唤醒后，subLoop执行注册好的回调函数
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    looping_ = false;
}

// 退出事件循环
// 1.loop在自己的线程中调用quit()，直接退出事件循环
// 2.loop在其他线程中调用quit()，需要通过wakeupfd唤醒loop所在的线程
void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup(); // 唤醒loop所在的线程
    }
}

// 在当前loop中执行回调函数
// 为什么会有这种设计？
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

// 把cb放到队列中，唤醒loop所在的线程执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    // 唤醒loop所在的线程
    // callingPendingFunctors_：当前loop正在执行回调但是loop又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

// 唤醒事件循环
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %zd bytes instead of 8\n", n);
    }
}

void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}

// 执行回调函数
void EventLoop::doPendingFunctors() {
    // 局部functors为了防止pendingFunctors_被一直占用导致主Reactor阻塞
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const auto& functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead() reads %zd bytes instead of 8\n", n);
    }
}