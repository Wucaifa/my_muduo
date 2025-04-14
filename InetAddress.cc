#include "InetAddress.h"

#include <strings.h>
#include <string.h>

// 创建InetAddress对象，初始化IPv4地址和端口号
InetAddress::InetAddress(uint16_t port, std::string ip) {
    bzero(&addr_, sizeof(addr_));
    // AF_INET表示IPv4地址
    addr_.sin_family = AF_INET;
    // htons将主机字节序转换为网络字节序
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

// 将IPv4地址转换为字符串格式
// inet_ntop将网络字节序转换为主机字节序
std::string InetAddress::toIp() const {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return std::string(buf);
}

// 将IPv4地址和端口号转换为字符串格式
std::string InetAddress::toIpPort() const {
    char buf[INET_ADDRSTRLEN + 6]; // IPv4地址 + 端口号 + '\0'
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ":%u", ntohs(addr_.sin_port));
    return std::string(buf);
}

// 将端口号转换为主机字节序
// ntohs将网络字节序转换为主机字节序
uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}

// 测试InetAddress类
// #include <iostream>
// int main(){
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;

//     return 0;
// }