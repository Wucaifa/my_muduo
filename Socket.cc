#include "Socket.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <netinet/tcp.h>


Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& addr) {
    if (::bind(sockfd_, (sockaddr*)addr.getSockAddr(), sizeof(struct sockaddr_in)) != 0) {
        LOG_FATAL("bind socketfd: %d fail\n", sockfd_);
    }
}

void Socket::listen() {
    if (::listen(sockfd_, 1024) != 0) {
        LOG_FATAL("listen socketfd: %d fail\n", sockfd_);
    }
}

int Socket::accept(InetAddress& addr) {
    sockaddr_in addrIn;
    socklen_t addrLen = sizeof(addrIn);
    bzero(&addrIn, sizeof(addrIn));
    int connfd = ::accept4(sockfd_, (sockaddr*)&addrIn, &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        LOG_ERROR("accept socketfd: %d error\n", sockfd_);
        return -1;
    }
    addr.setSockAddr(addrIn);
    return connfd;
}

void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) != 0) {
        LOG_ERROR("shutdown socketfd: %d error\n", sockfd_);
    }
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}