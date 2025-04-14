#pragma once 

#include <vector>
#include <cstddef> // For size_t
#include <string>

/**
 * +-------------------+-------------------+----------------------+
 * | prependable bytes  |  readable bytes    |  writable bytes      |
 * +-------------------+-------------------+----------------------+
 * | 8 bytes           | 0 bytes           | 1024 bytes           |
 * +-------------------+-------------------+----------------------+
 * |               readIndex_          writeIndex_                |
 */

class Buffer {
public:
    static const size_t kCheapPrepend = 8; // 8字节的预留空间
    static const size_t kInitialSize = 1024; // 初始大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize),
          readIndex_(kCheapPrepend),
          writeIndex_(kCheapPrepend) {}
    ~Buffer() = default;

    // 获取缓冲区的大小
    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writableBytes() const { return buffer_.size() - writeIndex_; }
    // 前面空闲的空间
    size_t prependableBytes() const { return readIndex_; }

    // 获取可读缓冲区的起始地址
    const char* peek() const { return begin() + readIndex_; } // 获取可读数据的起始地址

    void retrieve(size_t len) { // 读取len字节数据
        if (len < readableBytes()) {
            readIndex_ += len;
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() { // 读取所有数据
        readIndex_ = kCheapPrepend;
        writeIndex_ = kCheapPrepend;
    }

    // 把onmessage函数上的Buffer数据，转换成string返回
    std::string retrieveAllAsString() { // 读取所有数据并返回字符串
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString() { // 读取数据并返回字符串
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len) { // 读取len字节数据并返回字符串
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len) { // 确保可写空间足够
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }

    // 把数据写入缓冲区
    void append(const char* data, size_t len) { // 写入数据
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writeIndex_ += len;
    }

    char* beginWrite() { // 获取可写缓冲区的起始地址
        return begin() + writeIndex_;
    }

    ssize_t readFd(int fd, int* savedErrno); // 从fd读取数据

    ssize_t writeFd(int fd, int* savedErrno); // 向fd写入数据

private:
    char* begin() { return &buffer_[0]; } // 获取缓冲区的起始地址
    const char* begin() const { return &buffer_[0]; } // 获取缓冲区的起始地址

    void makeSpace(size_t len) { // 扩展缓冲区
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writeIndex_ + len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readIndex_, begin() + writeIndex_, begin() + kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = readIndex_ + readable;
        }
    }
private:
    std::vector<char> buffer_; // 缓冲区
    size_t readIndex_; // 读索引
    size_t writeIndex_; // 写索引
};