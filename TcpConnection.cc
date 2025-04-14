#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <unistd.h>

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d - TcpConnection Loop is null\n",
                  __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop,
                                       const std::string& name,
                                       int sockfd,
                                       const InetAddress& localAddr,
                                       const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(name),
      state_(StateE::kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr) ,
      highWaterMark_(64 * 1024 * 1024) {
        // 下面给channel设置回调函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG("TcpConnection::ctor[%s] at %p fd=%d\n",
              name_.c_str(), this, sockfd);
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection::dtor[%s] at %p fd=%d state=%d\n",
              name_.c_str(), this, channel_->fd(), state_.load());
}

void TcpConnection::shutdown() {
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void TcpConnection::shutdownInLoop() {
    if (!channel_->isWriting()) {
        socket_->shutdownWrite(); // 关闭写端
    }
}

void TcpConnection::connectEstablished() {
    LOG_DEBUG("TcpConnection::connectEstablished [%s] at %p\n",
              name_.c_str(), this);
    setState(StateE::kConnected);
    channel_->tie(shared_from_this()); // 绑定TcpConnection对象
    channel_->enableReading(); // 注册可读事件
    connectionCallback_(shared_from_this()); // 连接建立的回调
}

void TcpConnection::connectDestroyed() {
    if (state_ == StateE::kConnected) {
        setState(StateE::kDisconnected);
        channel_->disableAll(); // 取消所有事件
        connectionCallback_(shared_from_this()); // 连接关闭的回调
    }
    channel_->remove(); // 从poller中删除channel
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        // 已建立连接的用户，有可读事件发生了
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead [%s] - SO_ERROR = %d\n",
                  name_.c_str(), savedErrno);
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        int savedError = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedError);
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == StateE::kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite [%s] - SO_ERROR = %d\n",
                      name_.c_str(), errno);
        }
    } else {
        LOG_ERROR("TcpConnection::handleWrite [%s] - Channel is down, no more writing\n",
                  name_.c_str());
    }
}

void TcpConnection::handleClose() {
    LOG_DEBUG("TcpConnection::handleClose [%s] - state = %d\n",
              name_.c_str(), state_.load());
    setState(StateE::kDisconnected);
    channel_->disableAll();
    
    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis); // 连接关闭的回调
    closeCallback_(guardThis);  // 关闭连接的回调
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    }else{
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError [%s] - SO_ERROR = %d\n",
              name_.c_str(), err);
}

/**
 * 发送数据，应用写的快，内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
 */

void TcpConnection::sendInLoop(const void* message, size_t len) {
    ssize_t n = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == StateE::kDisconnected) {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 表示channel_第一次写数据，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        n = ::write(channel_->fd(), message, len);
        if (n >= 0) {
            remaining -= n;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            n = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop [%s] - SO_ERROR = %d\n",
                          name_.c_str(), errno);
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    // 说明这一次write没有把数据全部写完，剩余的数据需要放入缓冲区，然后给channel_
    // 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的channel调用writecallback会覅
    // 也就是调用TcpConnection::handleWrite函数，把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(message) + n, remaining);
        if (!channel_->isWriting()) {
            // 这里一定要注册epollout事件
            // 否则poller不会通知channel_，channel_也不会调用TcpConnection::handleWrite函数
            channel_->enableWriting();
        }
    }
}

void TcpConnection::send(const std::string& message) {
    if (state_ == StateE::kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message.c_str(), message.size());
        } else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, shared_from_this(), message.c_str(), message.size()));
        }
    }
}