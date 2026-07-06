# `src/comm` 通讯框架说明

## 1. 总览

`src/comm` 将通讯拆成三层：

```text
业务管理器（McuMgr / BtnMgr / TuyaMgr / ...）
        │ 发送 BuffData / 接收 IAck
        ▼
PacketChannel<TransportType>
        │ 编码包发送、字节流组包、校验、去重、命令分发
        ▼
Transport（TcpClient / TcpServer / UartClient）
        │ 连接管理与原始字节收发
        ▼
TCP socket / POSIX UART
```

各目录职责如下：

| 目录 | 主要类型 | 职责 |
| --- | --- | --- |
| `core/` | `Transport`、`AsyncTransport` | 统一原始通讯接口；将底层事件交给 `TransportHandler` |
| `tcp/` | `TcpClient`、`TcpServer` | TCP 连接、重连、监听、多客户端管理和收发线程 |
| `uart/` | `UartClient` | 串口配置、FD/Tick 两种读取方式和同步写入 |
| `packet/` | `IAsk`、`IAck`、`PacketBufferPool`、`PacketChannel`、`PacketManager` | 协议包编码、流式解码、缓存复用、去重和业务分发 |

`Transport` 不理解协议；`PacketChannel` 不直接处理业务；业务只需实现具体的 `IAsk`、`IAck` 和 `PacketHandler`。

---

## 2. 类关系与数据方向

```text
TransportHandler <-----------------------------+
    ^                                           |
    |                                           |
PacketChannel<TransportType>                    |
    | owns                                      | callback
    +-- TransportType --------------------------+
    |      +-- Transport
    |            +-- AsyncTransport (TCP 使用)
    |
    +-- PacketStreamDecoder
    |      +-- IAck（每条流独占）
    |      +-- 当前包 BuffData
    |      +-- 上一个有效包 BuffData
    |
    +-- PacketBufferPool（外部传入，不拥有）
             +-- PacketBufferPoolT<T, Ask, Ack>

PacketStreamDecoder --完整且校验通过--> PacketManager
PacketManager --按 IAck::getType()--> PacketHandler（业务管理器）
```

核心所有权：

- `PacketChannel` 按值持有底层 `TransportType` 和默认 `PacketStreamDecoder`。
- `PacketBufferPool` 由业务层创建并传入，必须比 `PacketChannel` 活得更久。
- `PacketStreamDecoder` 独占一个 `IAck`，并从池中占用“当前包”和“上一包”两个缓存，析构时归还。
- `PacketManager` 只保存裸 `PacketHandler*`，不拥有处理器；`PacketHandler` 析构时会自动注销。

---

## 3. 原始通讯层

### 3.1 `Transport` 接口

所有通道提供同一生命周期：

```cpp
transport.setHandler(handler);
int rc = transport.init();
bool ok = transport.start();
ssize_t sent = transport.send(data, len, id);
transport.stop();
```

事件模型：

| 事件 | 回调 | `id` 含义 |
| --- | --- | --- |
| `CONNECTED` | `onConnected(id)` | TCP 服务端为客户端 ID；点对点通道一般为 `-1` |
| `DISCONNECTED` | `onDisconnected(id)` | 同上 |
| `DATA` | `onRecv(data, len, id)` | 原始字节流，不保证对应一个协议包 |
| `ERROR` | `onError(err)` | errno 风格错误码；当前接口不向处理器传来源 ID |

注意：`onRecv` 的 `data` 只在本次回调中有效。回调中不能直接销毁当前 `Transport`、`PacketChannel` 或 handler，否则正在执行的分发栈会访问失效对象。

### 3.2 `AsyncTransport` 跨线程分发

TCP 的 socket 工作在线程中，而业务通常要求在 cdroid Looper 线程处理，因此 TCP 继承 `AsyncTransport`：

```text
TCP 工作线程
  postEvent()
      │
      ├─ 加入受互斥锁保护的 Event 队列
      └─ write(eventfd)
               │
               ▼
初始化时绑定的 cdroid::Looper
  onWake() -> drainWakeFd() -> dispatchPendingEvents()
                                  │
                                  ▼
                            TransportHandler
```

关键细节：

- `initAsyncDispatcher()` 使用当前线程的 `Looper::getForThread()`，因此 `init()` 必须在目标回调线程调用，而且该线程必须已有 Looper。
- 队列默认上限为 256，可通过 `setMaxPendingEventCount()` 调整。
- 队列满时生产线程最多等待 100 ms；仍无空间则丢弃当前事件，并增加 `droppedEventCount()`。数据、连接、断开和错误事件没有不同优先级，都可能被丢弃。
- 丢弃日志在第 1、2、4、8……次输出，避免满载时刷屏。
- Looper 每次唤醒会把共享队列整体交换到局部队列，再逐个回调，缩短持锁时间；交换后立即唤醒等待队列空间的生产线程。
- `cancelAsyncEvents()` 增加分发代数并清空共享队列。若它在某个事件回调中发生，当前局部批次在本次回调结束后停止继续分发。
- `stop()` 会在必要时 `flushAsyncEvents()`，用于保留断开回调；正常使用应从绑定 Looper 的线程执行 `stop()`，否则 flush 中的回调线程语义会发生变化。

---

## 4. TCP 通讯流程

### 4.1 `TcpClient`

#### 启动与连接

```text
init()
  ├─ 校验 host、port、重连间隔、发送超时
  └─ 创建 eventfd 并绑定当前 Looper

start()
  ├─ mRunning = true
  └─ 创建工作线程
        │
        ▼
      connectServer()
        ├─ 独立 detached 线程执行 getaddrinfo()
        ├─ 主工作线程每 50 ms 检查解析完成或停止请求
        ├─ 逐个尝试解析出的地址（IPv4/IPv6）
        ├─ 非阻塞 connect()
        └─ 每 100 ms poll(POLLOUT)，使 stop 可及时打断等待
```

连接成功后恢复 socket 原始 flags，将 FD 写入 `mSock`，设置 `mConnected`，再异步投递 `CONNECTED`。

DNS 解析本身不可取消。停止时等待方会把解析状态标记为 abandoned 并退出；解析线程稍后自行释放结果，不再访问客户端对象。

#### 接收、断开与重连

连接后的工作线程阻塞 `recv()`：

```text
recv() > 0          -> 复制字节到 Event::data -> post DATA
recv() == 0         -> 对端正常关闭
EINTR               -> 重试
EAGAIN/EWOULDBLOCK  -> 等待 10 ms 后重试
其他错误            -> post ERROR -> 结束本次连接
                          │
                          ▼
closeSocket() -> post DISCONNECTED -> 等待 reconnectDelayMs -> 重连
```

`reconnectDelayMs` 默认 2000 ms。条件变量让 `stop()` 可以立即打断重连等待。

#### 发送

- `send()` 用 `mSendLock` 串行化发送，并在 `mSocketLock` 下取得当前 FD。
- `sendAll()` 使用 `MSG_NOSIGNAL | MSG_DONTWAIT`，避免 SIGPIPE 并控制超时。
- 遇到发送缓冲区满时用 `poll(POLLOUT)` 等待剩余超时时间。
- 返回实际写入字节数；超时或错误可能返回小于请求长度的值。上层必须把“完整写入”作为成功条件。
- `closeSocket()` 也取得 `mSendLock`，避免关闭正在发送的 FD。

### 4.2 `TcpServer`

`start()` 创建 IPv4 `0.0.0.0:port` 监听 socket，启用 `SO_REUSEADDR`，调用 `listen(backlog)`，然后创建单个 poll 工作线程。启动成功还会投递一次 `CONNECTED(-1)`，其含义是“服务已开始监听”，不是客户端已经接入。

工作线程每轮：

1. 在锁内复制当前监听 FD 和 `id -> client fd` 映射。
2. 构造监听 socket 加所有客户端 socket 的 `pollfd` 快照。
3. 最多阻塞 500 ms。
4. 监听 FD 可读时 `accept()` 一个客户端，分配从 1 递增的 ID，投递 `CONNECTED(id)`。
5. 对本轮快照中的客户端读取数据，投递带 ID 的 `DATA`。
6. EOF、hangup 或不可恢复错误时关闭并移除客户端，再投递 `DISCONNECTED(id)`；有明确 socket 错误时会先投递 `ERROR`。

`send(data, len, id)` 必须指定有效客户端 ID，不提供广播语义。发送策略与 `TcpClient` 相同。`isConnected()` 对服务端表示“正在监听”，不表示存在客户端。

服务端的 poll 列表基于每轮快照：本轮新 accept 的客户端会从下一轮开始接收，这是有意的简化。`stop()` 会先关闭监听和全部客户端 FD，使 poll 退出，再 join 工作线程。

---

## 5. UART 通讯流程

### 5.1 初始化

`UartClient::init()` 的顺序是：

1. 校验设备路径。
2. 在进程级静态集合 `sUsedDevices` 中登记设备，禁止两个 `UartClient` 同时打开同一路径；重复占用会 `abort()`，不是普通错误返回。
3. 以 `O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC` 打开设备。
4. 使用 raw 模式配置波特率、数据位、停止位、奇偶校验和流控。
5. 设置 `VMIN=0`、`VTIME=0`。

每次 `start()` 启动读取驱动前都会调用 `tcflush(TCIOFLUSH)` 清空收发队列；清空失败时启动失败。

支持的常用波特率为 300～38400，并在平台宏存在时支持 57600、115200、230400、460800、921600。流控值：0 无流控、1 硬件流控、2 软件流控。

### 5.2 两种读取驱动

构造时根据 `pollIntervalMs` 固定选择模式：

| 条件 | 模式 | 行为 |
| --- | --- | --- |
| `< 100 ms` | Looper FD 模式 | 将串口 FD 注册到当前线程 Looper，收到 INPUT/ERROR/HANGUP 时立即处理 |
| `>= 100 ms` | Tick 模式 | 由 `TickMgr` 定期触发，使用零超时 `poll()` 检查串口 |

当前工程内 MCU、按钮、涂鸦通道均设置 `pollIntervalMs = 200`，使用 Tick 模式。FD 模式和 TCP 一样要求调用线程已经有 Looper，但 UART 不创建工作线程，读取和 handler 回调都直接发生在 Looper/Tick 所在线程。

`readAvailable()` 循环读取，直到：读取量小于缓冲区大小、暂时无数据、被信号打断或发生错误。每次成功 `read()` 都立即同步分发一个 `DATA`，所以一次回调可能是半包、一个包或多个粘连包。

`EIO`、`ENODEV`、`EBADF` 或 Looper 的 HANGUP/ERROR 会触发：停止读取驱动、关闭 FD、释放设备占用、依次分发 `ERROR` 和 `DISCONNECTED`。UART 不自动重连；业务若要恢复，需要再次 `init()/start()`。

### 5.3 UART 发送

`writeAll()` 在非阻塞 FD 上循环写入。`EAGAIN`、`EWOULDBLOCK`、`EINTR` 时每次 `poll(POLLOUT)` 10 ms 后重试：

- `writeTimeoutMs >= 0`：到达总超时后返回部分写入长度；默认 1000 ms。
- `writeTimeoutMs < 0`：无限等待，直到写完或出现不可恢复错误。
- 不可恢复错误同步分发 `ERROR`，但只有设备错误读取路径会自动断开通道。

当前 UART 实现没有独立发送锁，设计前提是调用方在同一线程串行发送；多线程同时调用 `send()` 会导致帧字节交错风险。

---

## 6. 协议包模型

### 6.1 `BuffData`

```cpp
#pragma pack(1)
struct BuffData {
    int16_t  type;  // 框架内协议类型，不会自动发送到线路
    uint16_t len;   // 当前有效字节数
    uint16_t slen;  // buf 容量
    uint8_t  buf[1];// 尾随分配的数据区
};
```

分配大小为 `sizeof(BuffData) + capacity`。由于结构本身已有 `buf[1]`，实际分配比严格最小值多一个字节，但对使用逻辑无影响。线路上发送的只有 `buf[0..len)`，`type/len/slen` 都是内存管理元数据。

### 6.2 发送编码器 `IAsk`

具体协议继承 `IAsk` 并提供：

- `BASE_LEN`：固定帧开销或固定帧长度。
- `parse()`：通常调用基类后写入包头和默认字段。
- `checkCode()`：在业务字段写完后生成校验字节。

`IAsk::parse()` 在新缓存 `len == 0` 时将 `len` 设置为整个 `slen`，因此默认认为取得的发送缓存全部有效。`setData()` 按 `slen` 做边界检查，不会自动改变 `len`。

典型发送流程：

```cpp
BuffData* packet = pool->obtainSend(payloadLen);
ConcreteAsk ask(packet);        // 绑定缓存并写固定头
ask.setData(...);               // 写版本、命令、长度、载荷等
ask.checkCode();                // 最后生成校验
channel->send(packet);          // 无论成功失败，PacketChannel 都会回收 packet
```

一旦把 `BuffData*` 传给 `PacketChannel::send()`，调用方即失去所有权，不得再次访问或回收。若在编码阶段提前返回、尚未调用 channel，则调用方必须自行 `pool->recycle(packet)`。

### 6.3 接收解码器 `IAck`

具体协议必须给出：

| 接口 | 含义 |
| --- | --- |
| `head()` / `headLength()` | 包头字节及长度 |
| `lengthReadySize()` | 至少收到多少字节后才能解析完整帧长 |
| `expectedLength()` | 根据当前缓存返回完整帧总长度 |
| `check()` | 完整包校验 |
| `getType()` | `PacketManager` 使用的业务类型 |

固定长度协议可令 `lengthReadySize() == headLength()`，`expectedLength()` 返回常量；变长协议应使 `lengthReadySize()` 覆盖完整长度字段，再由 `expectedLength()` 读取它。

`IAck::add()` 是状态化的流解析器，处理流程如下：

```text
输入任意一段字节流
  │
  ├─ 逐字节寻找协议头
  │    ├─ 丢弃头前噪声
  │    └─ 输入末尾若是包头前缀则保留，支持跨回调包头
  │
  ├─ 批量复制到 lengthReadySize()
  │
  ├─ expectedLength()
  │    ├─ < lengthReadySize() 或 > 缓存容量
  │    │      -> 丢掉当前包头第一个字节并重新同步
  │    └─ 合法
  │
  └─ 只复制到本帧 expectedLength()
       ├─ 不完整：保留状态，等待下次输入
       └─ 完整：返回本轮消费量，粘连的后续字节留给上层下一轮解析
```

这套逻辑同时覆盖：噪声前缀、包头跨 read、包体拆包、多包粘连、非法长度后重新同步。接收缓存满且未形成有效帧时会清空并继续寻头。

`hasRange()` 统一检查目标范围是否位于完整包的 `mDataLen` 内；校验位也属于完整包，可以正常读取。`readU8()`、`readU16()`、`readU32()` 和 `readU64()` 通过返回值区分读取失败与数值 0，多字节读取使用 `IAck::ByteOrder::BigEndian` 或 `IAck::ByteOrder::LittleEndian` 明确线路字节序（该类型是 `EncodingUtils::ByteOrder` 的别名）。`readBytes()` 返回指定范围的只读视图，指针仅在当前数据包回调期间有效。

---

## 7. 缓存池

`PacketBufferPoolT<T, Ask, Ack>` 将协议类型、发送编码器和接收解码器绑定：

```cpp
using TuyaPool = PacketBufferPoolT<BT_TUYA, TuyaAsk, TuyaAck>;
```

- `obtainSend(dataLen)` 分配 `Ask::BASE_LEN + dataLen` 容量。
- `obtainReceive()` 分配 `Ack::BUFFER_CAPACITY` 容量。
- `createAck()` 为每条数据流创建独立解析器。
- 缓存仅在 `type` 和 `slen` 同时匹配时复用，避免不同协议或不同长度误用。
- `recycle()` 清零数据区和 `len`。默认最多缓存 16 个空闲包；超过上限直接 `free()`。
- `setMaxCacheCount(0)` 可禁用空闲缓存；调小上限会立即释放超额项。
- 长度使用 `uint16_t`，基础长度与可变长度之和不得超过 65535。

缓存池没有互斥保护。其分配、回收、销毁必须遵守同线程模型；特别是不要让发送线程与回调线程同时操作同一个池。

---

## 8. `PacketChannel` 接收流程

### 8.1 完整链路

```text
socket/read()
  -> Transport DATA
  -> PacketChannel::onRecv(data, len, id)
  -> PacketStreamDecoder::onBytes()
  -> IAck::add()
       ├─ 本次未成包：保存部分数据并返回
       └─ 得到一个完整包
            ├─ recvCount++（包括校验失败包）
            ├─ IAck::check()
            │    └─ 失败：checkErrorCount++，记录 hexdump
            └─ 成功：checkErrorCount = 0
                 ├─ 重复过滤
                 └─ PacketManager::onCommand(ack, id)
                       -> PacketHandler::onCommDeal(ack, id)
```

`PacketChannel::onRecv()` 使用 offset 循环调用解码器，因此一个底层 DATA 中的多个完整包会依次分发。解码器每完成一包就把当前缓存 `len` 清零，但保留尚未完成的半包状态。

`recvCount()` 表示识别出的完整帧数，包括校验失败和被去重的帧；`sendCount()` 只统计完整发送成功的协议包；`lastError()` 记录最近一次底层错误。

### 8.2 连续重复包过滤

解码器保留最后一个“校验通过且已接收”的包。`enableRepeatAccept == false` 时，新包与上一包满足以下条件即不再业务分发：

- `BuffData::type` 相同；
- `len` 相同；
- 全部线路字节完全相同。

只有连续相同包被过滤；`A, B, A` 中第二个 A 仍会处理。校验失败包不会更新上一包。连接断开时默认解码器会 `reset()`，清除半包、上一包和连续校验失败计数，但保留累计接收数。

工程当前配置中：

- MCU、按钮通道传入 `false`，过滤连续重复包。
- 涂鸦通道传入 `true`，心跳、OTA 等相同帧仍会逐次处理。

### 8.3 客户端 ID 模式

编译宏 `PACKET_CHANNEL_ENABLE_MULTI_ID` 默认是 0：

- 点对点 TCP 客户端和 UART 的 `id == -1` 正常使用默认解码器。
- 若接到非负 ID，第一个 ID 会成为 `mSingleClientId`；其他客户端的连接、数据和断开事件被拒绝并记录错误。
- 这意味着默认配置下 `PacketChannel<TcpServer>` 实际只适合同时解析一个客户端。原始 `TcpServer` 本身仍支持多客户端。

宏设为 1 后，每个非负 ID 都创建独立 `PacketStreamDecoder`，避免不同 TCP 客户端的字节流混入同一半包状态；断开时删除对应解码器。此模式下缓存池至少会长期占用每客户端两个接收缓存，应据最大连接数调整缓存和内存预算。

---

## 9. `PacketManager` 业务分发

`PacketManager` 是单例，按 `IAck::getType()` 保存一组处理器，支持：

- 一个包类型对应多个处理器；
- 一个处理器注册多个包类型；
- 拒绝同一类型下重复注册同一指针；
- 处理器析构时从所有类型中注销。

分发时先复制处理器列表快照。每次回调前再确认类型和处理器仍存在，因此回调中移除自身或其他处理器是安全的；回调中新增的处理器要等下一包才生效。

默认的 `PacketHandler::onCommDeal(ack, id)` 会转调无 ID 重载，所以点对点业务只需覆盖 `onCommDeal(const IAck*)`；服务端业务可覆盖带 ID 版本并把该 ID 原样用于回复。

管理器明确不支持并发。注册、移除和 `onCommand()` 必须在同一线程执行。当前 TCP 通过 `AsyncTransport` 回归 Looper、UART 直接运行在 Looper/Tick 线程，正是为了满足这一约束。

`IAck*` 及其 `data()` 只在 `onCommDeal()` 调用期间有效。业务需要异步保存时必须复制数据，不能保存指针。

---

## 10. 工程中的实际接入

当前 `McuMgr`、`BtnMgr`、`TuyaMgr` 都遵循相同模式：

```text
构造管理器
  └─ new PacketBufferPoolT<业务类型, Ask, Ack>

init()
  ├─ PRODUCT_X64：构造 TcpClient::Config
  │                  调试服务基础端口 + 业务类型
  ├─ 非 PRODUCT_X64：构造 UartClient::Config
  │                  /dev/ttyS1、S2 或 S3，9600 8N1
  ├─ new PacketChannel<选定 Transport>(pool, 去重配置, config)
  ├─ channel->init()（底层 init + start）
  ├─ PacketManager::addHandler(业务类型, this)
  └─ 启动业务 Tick
```

同一套协议层因此可以在 X64 调试环境走 TCP，在设备环境走 UART，业务编码和解析逻辑不变。

发送示例来自涂鸦通道：申请 `BASE_LEN + payloadLen`、构造 `TuyaAsk` 写包头、填版本/命令/长度/载荷、生成校验，最后交给 channel 自动回收。接收时 `TuyaAck` 根据字节 4～5 读取大端载荷长度，以 `payloadLen + 7` 判定完整帧，再由命令字段进入业务 switch。

`page_test_tcp` 展示了不经过 packet 层的原始用法：分别给 `TcpServer`、`TcpClient` 设置 `TransportHandler`，服务端保存连接回调提供的客户端 ID，发送时将 ID 传回 `TcpServer::send()`。

---

## 11. 生命周期与线程约束

推荐销毁顺序：

```text
停止业务 Tick/禁止新发送
  -> PacketChannel::stop()
  -> 销毁 PacketChannel
  -> 注销/销毁 PacketHandler
  -> 销毁 PacketBufferPool
```

工程管理器通过先删除 channel、后删除 pool 满足关键所有权约束；处理器基类析构时自动从 `PacketManager` 注销。

需要特别遵守：

1. `PacketBufferPool` 生命周期必须覆盖 channel 和所有 decoder。
2. 不在通讯回调中同步销毁 channel、transport 或当前 handler；应投递延迟任务。
3. TCP 的 `init/start/stop` 尽量都在绑定的 Looper 线程调用。
4. 不并发调用 `PacketManager`，也不并发操作同一 `PacketBufferPool`。
5. UART 多线程发送必须在上层串行化。
6. TCP 服务端回复必须保存并使用当前来源 ID；不要把旧 ID 当作永久会话标识。
7. `PacketChannel::send(BuffData*)` 总会消费并回收参数，包括未连接和发送失败的情况。

---

## 12. 配置与返回值速查

### TCP Client

| 配置 | 默认值 | 说明 |
| --- | ---: | --- |
| `host` | 空 | 必填，域名或 IP |
| `port` | 0 | 必填 |
| `reconnectDelayMs` | 2000 | 断开/连接失败后的重试间隔；0 表示立即重试 |
| `sendTimeoutMs` | 1000 | 必须大于 0 |
| `readBufferSize` | 4096 | 0 时仍使用 4096 |

### TCP Server

| 配置 | 默认值 | 说明 |
| --- | ---: | --- |
| `port` | 0 | 必填 |
| `backlog` | 8 | `listen()` 队列长度 |
| `sendTimeoutMs` | 1000 | 必须大于 0 |
| `readBufferSize` | 4096 | 0 时仍使用 4096 |

### UART

| 配置 | 默认值 | 说明 |
| --- | ---: | --- |
| `device` | 空 | 必填 |
| `baudRate` | 9600 | 必须是平台支持的映射值 |
| `flowControl` | 0 | 0 无、1 硬件、2 软件 |
| `dataBits` | 8 | 5/6/7/8 |
| `stopBits` | 1 | 1/2 |
| `parity` | `N` | N/O/E，不区分大小写 |
| `pollIntervalMs` | 200 | `<100` 使用 FD，否则 Tick |
| `writeTimeoutMs` | 1000 | 负数表示无限等待 |
| `readBufferSize` | 4096 | 0 时仍使用 4096 |

通用返回语义：

- `init()`：0 成功，非 0 失败，具体负值用于区分配置或资源阶段。
- `start()`：成功为 `true`，重复启动幂等。
- `stop()`：重复停止幂等。
- 原始 `send()`：返回实际字节数；参数、连接或目标无效时为 -1；部分发送不是成功。
- `PacketChannel::send(BuffData*)`：完整发送为 0，否则 -1，并始终回收数据包。

---

## 13. 已知边界与扩展建议

以下是使用或扩展时需要明确的设计边界：

- `TransportHandler::onError()` 没有 ID。TCP 服务端能在内部事件中记录错误来源，但分发到 handler 时来源会丢失；需要精确定位客户端错误时应扩展接口。
- TCP 事件队列满会无差别丢事件。若数据不可丢，应增加背压、提高队列容量，或设计数据专用缓冲；只查看错误日志不足以恢复字节流。
- `TcpServer` 当前只监听 IPv4 `INADDR_ANY`，没有 IPv6、TLS、访问控制和最大客户端数限制。
- TCP 客户端没有单次连接总超时；只要运行标志为真，非阻塞连接会按 100 ms 片段继续等待，最终依赖系统 socket 连接结果。
- UART 重复设备是 fail-fast `abort()`。若设备冲突属于可恢复场景，应改为普通错误码。
- UART 写路径不保证多线程安全，TCP 写路径则有发送锁。
- 包去重是完整字节比较，不是序列号或业务幂等机制；不能替代请求应答匹配。
- 当前 `PacketManager` 以全局包类型路由。如果多个同类型通道同时存在，它们会进入相同处理器；需要按通道隔离时应增加 channel/source 维度。
- `PacketBufferPool` 和计数器不是线程安全对象。若未来把解析或业务分发迁移到线程池，需要先补充同步或改为单线程 actor 模型。
- `IAck::add()` 接口长度为 `int`，而上层长度为 `size_t`。正常 socket/UART 读取缓存远小于 `INT_MAX`；若未来直接注入超大内存块，应先分片。

新增协议时，优先保持现有分层：只在 `Ask/Ack` 中描述帧格式，在 `PacketHandler` 中处理业务，不把心跳、命令语义或校验规则下沉到 `Transport`。
