mymuduo

基于陈硕《Linux 多线程服务器端编程》一书所重构的 Muduo 网络库项目。

📚 项目背景

Muduo 是陈硕开发的一个高性能网络库，广泛用于 Linux 下多线程服务器的开发。本项目旨在通过学习并重构 Muduo 的核心模块，加深对 Reactor 模式、线程池、非阻塞 IO、定时器等关键技术的理解。

本项目在 Ubuntu 环境下使用 VSCode 远程开发完成。

🧩 项目结构

```bash
mymuduo/
├── muduo                   # 网络库核心模块
│   ├── base                # 基础设施：日志、时间戳、线程等
│   └── net                 # 网络模块：EventLoop、TcpServer 等
├── example                 # 使用示例，如 echo server/client
├── CMakeLists.txt          # 构建配置
└── README.md               # 项目说明文档
````
🔧 编译方法
本项目使用 CMake 进行构建，推荐使用以下步骤：
```bash
git clone git@github.com:nohappykedaya/mymuduo.git
cd mymuduo
mkdir build && cd build
cmake ..
make
```
🚀 示例运行

构建完成后，可进入 `example` 目录运行相关示例：

```bash
cd ../example/testserver
./testserver
```

📌 主要特性

* 基于 Reactor 的事件驱动模型
* 封装 Epoll + 非阻塞 IO
* 使用线程池提升并发性能
* 支持定时器、Buffer、连接管理等基础能力
* 代码结构更清晰，易于初学者理解和扩展

💻 开发环境

* OS：Ubuntu 20.04+
* 编译器：g++ 9+
* 构建工具：CMake 3.10+
* 编辑器：VSCode + Remote - SSH 插件

📖 参考资料

* 陈硕. 《Linux 多线程服务器端编程：使用 muduo C++ 网络库》. 2013.
* [muduo 原始项目](https://github.com/chenshuo/muduo)
