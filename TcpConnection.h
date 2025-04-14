#pragma once

#include "noncopyable.h"
#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer =》Acceptor =》有一个新用户连接，通过accept函数拿到connfd
 * =》TcpConnection 设置回调 =》 Channel =》 Poller =》 Channel的回调操作
 * 负责处理连接的读写操作
 */

class TcpConnection : Noncopyable, public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop,
                  const std::string& name,
                  int sockfd,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; } // 获取所属的EventLoop
    const std::string& name() const { return name_; } // 获取TcpConnection的名字
    const InetAddress& localAddress() const { return localAddr_; } // 获取本地地址
    const InetAddress& peerAddress() const { return peerAddr_; } // 获取对端地址

    bool connected() const { return state_ == StateE::kConnected; } // 是否连接成功

    void send(const std::string& message); // 发送数据
    void shutdown(); // 关闭连接

    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    void connectEstablished(); // 连接建立
    void connectDestroyed(); // 连接销毁

private:
    enum StateE {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting,
    };
    void handleRead(Timestamp receiveTime); // 处理读事件
    void handleWrite(); // 处理写事件
    void handleClose(); // 处理关闭事件
    void handleError(); // 处理错误事件

    void sendInLoop(const void* message, size_t len); // 在loop线程中发送数据
    void shutdownInLoop(); // 在loop线程中关闭连接

    void setState(StateE s) { state_ = s; } // 设置状态

private:

    EventLoop* loop_; // TcpConnection是在subLop上管理的
    const std::string name_; // TcpConnection的名字
    std::atomic<StateE> state_; // TcpConnection的状态
    bool reading_; // 是否在读取数据
    std::shared_ptr<Socket> socket_; // socket对象
    std::shared_ptr<Channel> channel_; // channel对象

    const InetAddress localAddr_; // 本地地址
    const InetAddress peerAddr_; // 对端地址

    ConnectionCallback connectionCallback_; // 连接回调函数
    MessageCallback messageCallback_; // 消息回调函数
    WriteCompleteCallback writeCompleteCallback_; // 写完成回调函数
    HighWaterMarkCallback highWaterMarkCallback_; // 水位线回调函数
    CloseCallback closeCallback_; // 关闭连接回调函数

    size_t highWaterMark_; // 水位线

    Buffer inputBuffer_; // 输入缓冲区
    Buffer outputBuffer_; // 输出缓冲区
};