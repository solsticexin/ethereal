# eirian — C++20 Reactor 网络库（Epoll + EventLoop）

`eirian` 是一个基于 Linux `epoll` 的轻量级 Reactor 网络库。项目实现了事件循环、Channel 事件分发、非阻塞 Socket 封装、缓冲区、TCP 连接与服务器等核心组件，支持回调驱动的高并发 IO 编程。

## 亮点
1. **Reactor 核心链路完整**：`Epoll -> EventLoop -> Channel -> 回调`
2. **非阻塞 IO + 输出缓冲区**：`TcpConnection::send` 自动处理短写并开启 `EPOLLOUT`
3. **可复用的 TCP 组件**：`TcpServer` 管理连接生命周期，`TcpConnection` 处理读写/关闭
4. **清晰的工程结构**：`include/` 暴露公共接口，`src/` 实现，`test/` 覆盖主要模块

## 模块结构
1. `Epoll`：对 `epoll_*` 系统调用的最小封装，提供 `add/modify/remove/poll`
2. `EventLoop`：事件循环引擎，维护 `fd -> Channel*` 映射
3. `Channel`：封装 `fd` 关注事件 + 触发回调
4. `Socket` / `InetAddress`：TCP socket 与地址封装
5. `Buffer`：字符串缓冲区，处理读写
6. `TcpConnection`：连接对象，管理读写、缓冲区、关闭回调
7. `TcpServer`：监听与连接管理（可设置消息回调）
8. `EventLoopThread`：子线程中启动 `EventLoop`

## 构建
要求：Linux、C++20、CMake 4.1+

```bash
cmake -S . -B build
cmake --build build -j
```

构建产物位于 `./bin`。

## 运行示例
### 1. 运行内置测试服务器（手写 Channel 回调）
```bash
./bin/eirian_app
```
默认绑定 `127.0.0.1:8888`，可用 `telnet`/`nc` 进行回声测试。

### 2. TcpServer 方式（推荐）
下面是项目中可用的典型用法：

```cpp
eirian::EventLoop loop;
eirian::TcpServer server(&loop, eirian::InetAddress("127.0.0.1", 8888));
server.setMessageCallback([](const std::shared_ptr<eirian::TcpConnection>& conn, std::string& msg) {
    conn->send(msg); // echo
});
server.start();
loop.loop();
```

## 测试
项目内 `test/` 目录覆盖 `Buffer/Channel/Epoll/EventLoop/TcpConnection/...` 等组件。

```bash
ctest --test-dir build
```

## 技术栈
1. C++20
2. Linux `epoll`
3. `spdlog` 日志库（CMake FetchContent 自动拉取）

## 设计取舍与当前状态
1. `EventLoop` 当前为单线程事件循环
2. `EventLoopThread` 已实现线程内 `loop()`，尚未提供 `quit()` 安全退出接口
3. 暂未提供 Timer、线程池、任务队列等高级功能

## 目录结构
```
.
├── include/eirian     # 头文件
├── src                # 实现
├── test               # 单元/组件测试
├── main.cpp           # 示例服务
└── CMakeLists.txt
```

## 适合简历的项目描述（可直接使用）
> 实现了基于 Linux epoll 的 C++20 Reactor 网络库，包含 EventLoop、Channel、非阻塞 Socket、缓冲区、TcpConnection/TcpServer 等核心组件；支持回调驱动的异步读写，具备输出缓冲区与 EPOLLOUT 处理，完成基础 TCP 回声服务器与多模块测试。

