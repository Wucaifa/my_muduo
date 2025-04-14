#pragma once

#include "noncopyable.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <unordered_map>

class TcpServer : Noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) {
        threadInitCallback_ = cb;
    }

    // 设置连接回调函数
    void setConnectionCallback(const ConnectionCallback &cb) {
        connectionCallback_ = cb;
    }

    // 设置消息回调函数
    void setMessageCallback(const MessageCallback &cb) {
        messageCallback_ = cb;
    }

    // 设置写完成回调函数
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        writeCompleteCallback_ = cb;
    }

    void setThreadNum(int numThreads) {
        threadPool_->setThreadNum(numThreads);
    }

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;    // baseloop 用户定义的loop
    const std::string name_; // 服务器名称
    const std::string ipPort_; // 服务器ip
    std::unique_ptr<Acceptor> acceptor_; // 监听socket,运行在mainloop
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池

    ConnectionCallback connectionCallback_; // 连接回调函数
    MessageCallback messageCallback_; // 消息回调函数
    WriteCompleteCallback writeCompleteCallback_; // 写完成回调函数

    ThreadInitCallback threadInitCallback_; // 线程初始化回调函数
    std::atomic_int started_; // 服务器是否启动

    int nextConnId_; // 下一个连接id
    ConnectionMap connections_; // 连接map
};