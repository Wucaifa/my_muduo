#pragma once

#include "noncopyable.h"
#include "InetAddress.h"

// 封装socketfd
class Socket : Noncopyable {
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

    int fd() const { return sockfd_; }

    void bindAddress(const InetAddress& addr);
    void listen();
    int accept(InetAddress& addr);
    void shutdownWrite();

    // 设置socket选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};