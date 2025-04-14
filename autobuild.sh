#!/bin/bash

set -e

# 创建 build 目录
if [ ! -d build ]; then
    mkdir build
fi

# 清空 build 目录
rm -rf "$(pwd)/build/*"

# 进入 build 目录并编译
cd "$(pwd)/build"
cmake ..
make

# 回到项目根目录
cd ..

# 安装头文件到 /usr/include/mymuduo
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

# 拷贝头文件
for header in *.h; do
    cp "$header" /usr/include/mymuduo
done

# 拷贝库文件
cp "$(pwd)/lib/libmymuduo.so" /usr/lib

# 刷新动态链接库缓存
ldconfig
