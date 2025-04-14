#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>
#include <memory>

/**
 * Acceptor类
 * 1. 负责监听新连接,listenfd的封装
 * 2. 负责接收新连接，封装成channel
 * 3. 负责分发新连接，分发给subLoop
 */

class EventLoop;
class InetAddress;

class Acceptor : Noncopyable {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = std::move(cb); }
    void listen();
    bool listening() const { return listening_; }

private:
    void handleRead();
private:
    EventLoop* loop_;   // Acceptor用的就是用户定义的那个baseLoop
    Socket acceptSocket_; // 监听socket
    Channel acceptChannel_; // 监听socket的channel
    NewConnectionCallback newConnectionCallback_; // 新连接的回调函数
    bool listening_; // 是否在监听 
};