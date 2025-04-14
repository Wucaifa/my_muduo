#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址
class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const struct sockaddr* getSockAddr() const { return (const struct sockaddr*)&addr_; }
    void setSockAddr(const struct sockaddr_in& addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};