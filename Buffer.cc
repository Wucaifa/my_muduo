#include "Buffer.h"

#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd中读取数据 Poller工作在LT模式下
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据的大小
 */
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    char extrabuf[65536];   // 栈上分配一个65536字节(64K)的缓冲区
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    // 如果可写缓冲区的大小大于64K，则只读一个缓冲区
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        writeIndex_ += n;
    } else {
        // Buffer缓冲区写满了，开始写入extrabuf
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno) {
    // 为什么把可读缓冲区的内容写入fd中？
    // 因为这是用于outputBuffer的writeFd函数，我们把数据写入，fd读取
    size_t n = ::write(fd, peek(), readableBytes());
    if (n <= 0) {
        *savedErrno = errno;
    }
    return n;
}