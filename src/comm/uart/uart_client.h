/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/uart/uart_client.h
 * @Description: Raw UART client
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#ifndef __UART_CLIENT_H__
#define __UART_CLIENT_H__

#include "transport.h"

#include <mutex>
#include <set>
#include <string>

/**
 * @brief 基于 POSIX 串口设备的原始字节通讯实现。
 *
 * 类内部直接完成设备打开和 termios 配置。当前实现不创建接收线程，使用者需要在
 * 所在线程周期调用 onTick() 驱动非阻塞读取。
 */
class UartClient : public Transport {
public:
    /** @brief 串口设备及收发参数。 */
    struct Config {
        /** @brief 串口设备路径，例如 /dev/ttyS1。 */
        std::string device;
        /** @brief 波特率，必须是平台 termios 支持的值。 */
        int baudRate{ 9600 };
        /** @brief 流控模式：0 无流控，1 硬件流控，2 软件流控。 */
        int flowControl{ 0 };
        /** @brief 数据位，可选 5、6、7、8。 */
        int dataBits{ 8 };
        /** @brief 停止位，可选 1、2。 */
        int stopBits{ 1 };
        /** @brief 校验位：N/n 无校验，O/o 奇校验，E/e 偶校验。 */
        char parity{ 'N' };
        /** @brief onTick() 两次实际读取检查之间的最小间隔，单位毫秒。 */
        int pollIntervalMs{ 10 };
        /** @brief 写入繁忙时的最大等待时间，单位毫秒；负数表示不设超时。 */
        int writeTimeoutMs{ 1000 };
        /** @brief 单次读取使用的缓存大小；为 0 时使用 4096 字节。 */
        size_t readBufferSize{ 4096 };
    };

public:
    /** @brief 保存配置；设备在 init() 或首次 start() 时打开。 */
    explicit UartClient(const Config& config);
    ~UartClient();

    /** @copydoc Transport::setHandler */
    void setHandler(TransportHandler* handler) override;
    /** @brief 打开并配置串口设备，成功返回 0。 */
    int init() override;
    /** @brief 启用串口通道并同步发出连接事件。 */
    bool start() override;
    /** @brief 停止通道、关闭设备并同步发出断开事件。 */
    void stop() override;
    /** @brief 设备已打开且通道已启动时返回 true。 */
    bool isConnected() const override;
    /** @brief 按 pollIntervalMs 周期非阻塞读取串口并同步分发数据。 */
    void onTick() override;
    /** @brief 写入原始字节；id 对串口无意义，会被忽略。 */
    ssize_t send(const uint8_t* data, size_t len, int id = -1) override;

protected:
    void dispatchEvent(const Event& ev) override;

private:
    int openDevice();
    int configureDevice(int fd);
    void releaseDeviceClaim();
    ssize_t writeAll(const uint8_t* data, size_t len);

private:
    static std::mutex sDeviceLock;
    static std::set<std::string> sUsedDevices;

    Config mConfig;
    int mFd;
    bool mRunning;
    bool mDeviceClaimed;
    int64_t mLastPollMs;
    TransportHandler* mHandler;
};

#endif // !__UART_CLIENT_H__
