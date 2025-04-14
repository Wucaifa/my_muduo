#pragma once

/**
 * @brief Noncopyable
 * @details 该类禁止拷贝构造和赋值操作，确保派生类不能被复制
 * @note 该类的构造函数和析构函数是默认的
 */

class Noncopyable {
 public:
    Noncopyable() = default;
    ~Noncopyable() = default;

 protected:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};