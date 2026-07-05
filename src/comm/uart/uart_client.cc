/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26 00:47:22
 * @LastEditTime: 2026-07-05 20:57:38
 * @FilePath: /kk_frame/src/comm/uart/uart_client.cc
 * @Description: 串口通讯客户端
 * @BugList:
 *
 * Copyright (c) 2026 by Ricken, All Rights Reserved.
 *
**/

#include "uart_client.h"

#include <cdlog.h>
#include <core/systemclock.h>

#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#define FailFast(condition, fmt, ...)   \
    do {                                \
        if (condition) {                \
            LOGE(fmt, ##__VA_ARGS__);   \
            std::abort();               \
        }                               \
    } while (0)

namespace {

    constexpr short UART_POLL_ERROR_EVENTS = POLLERR | POLLHUP | POLLNVAL;
    constexpr int   UART_FD_MODE_THRESHOLD_MS = 100;

    speed_t baudToTermios(int baudRate) {
        switch (baudRate) {
        case 300: return B300;
        case 1200: return B1200;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
#ifdef B57600
        case 57600: return B57600;
#endif
#ifdef B115200
        case 115200: return B115200;
#endif
#ifdef B230400
        case 230400: return B230400;
#endif
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
        default: {
            LOGE("Unsupported baud rate: %d!!!!!!", baudRate);
        }   return static_cast<speed_t>(0);
        }
    }

}

std::mutex UartClient::sDeviceLock;              // 设备占用记录锁
std::set<std::string> UartClient::sUsedDevices;  // 已占用的设备路径（防止重复打开同一设备）

/// @brief 串口通讯客户端构造
/// @param config 串口配置
UartClient::UartClient(const Config& config)
    : mConfig(config),
    mUseFdMode(config.pollIntervalMs < UART_FD_MODE_THRESHOLD_MS) {
    if (!mUseFdMode) {
        setTick(mConfig.pollIntervalMs);
    }
}

/// @brief 串口通讯客户端析构
UartClient::~UartClient() {
    stop();
}

/// @brief 打开并配置串口设备
/// @return 0: 成功, 非0: 失败
int UartClient::init() {
    if (mConfig.device.empty()) {
        LOGE("UartClient init failed: device is empty");
        return -1;
    }

    if (mFd >= 0) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(sDeviceLock);
        const bool inserted = sUsedDevices.insert(mConfig.device).second;
        FailFast(!inserted, "UartClient device already in use. device=%s",
            mConfig.device.c_str());
        mDeviceClaimed = true;
    }

    mFd = openDevice();
    if (mFd < 0) {
        releaseDeviceClaim();
        return -2;
    }

    if (configureDevice(mFd) != 0) {
        close(mFd);
        mFd = -1;
        releaseDeviceClaim();
        return -3;
    }

    tcflush(mFd, TCIOFLUSH);
    LOGI("UartClient init. device=%s baud=%d data=%d stop=%d parity=%c",
        mConfig.device.c_str(),
        mConfig.baudRate,
        mConfig.dataBits,
        mConfig.stopBits,
        mConfig.parity);
    return 0;
}

/// @brief 启用串口通道
/// @return true: 成功, false: 失败
bool UartClient::start() {
    if (mRunning) {
        return true;
    }

    if (mFd < 0 && init() != 0) {
        return false;
    }

    mRunning = true;
    if (!startReadDriver()) {
        mRunning = false;
        return false;
    }

    Event ev;
    ev.type = Event::CONNECTED;
    dispatchEvent(ev);
    return mRunning;
}

/// @brief 停止串口通道
void UartClient::stop() {
    const bool notifyDisconnected = mRunning;
    stopReadDriver();
    mRunning = false;
    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
    releaseDeviceClaim();

    if (notifyDisconnected) {
        Event ev;
        ev.type = Event::DISCONNECTED;
        dispatchEvent(ev);
    }
}

/// @brief 发送数据
/// @param data 数据
/// @param len 数据长度
/// @param id 客户端标识（串口通讯不需要）
/// @return 实际发送字节数，参数或通道状态无效时返回 -1；超时或写入错误时可能小于 len
ssize_t UartClient::send(const uint8_t* data, size_t len, int /*id*/) {
    if (!isConnected() || data == nullptr || len == 0) {
        return -1;
    }
    return writeAll(data, len);
}

/// @brief 通道是否已连接
/// @return true: 已连接, false: 未连接
bool UartClient::isConnected() const {
    return mRunning && mFd >= 0;
}

/// @brief 定时器回调
/// @param nowMs 当前时间戳
/// @note 长轮询间隔时触发
void UartClient::onTick(int64_t /*nowMs*/) {
    if (!isConnected()) {
        return;
    }

    pollfd pfd;
    pfd.fd = mFd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ready = poll(&pfd, 1, 0);
    if (ready < 0) {
        if (errno != EINTR) {
            Event ev;
            ev.type = Event::ERROR;
            ev.error = errno;
            dispatchEvent(ev);
        }
        return;
    }

    if (ready == 0) {
        return;
    }

    if ((pfd.revents & UART_POLL_ERROR_EVENTS) != 0) {
        int error = EIO;
        if ((pfd.revents & POLLNVAL) != 0) {
            error = EBADF;
        } else if ((pfd.revents & POLLHUP) != 0) {
            error = ENODEV;
        }
        disconnectOnDeviceError(error, pfd.revents);
        return;
    }

    if ((pfd.revents & POLLIN) == 0) {
        return;
    }

    readAvailable();
}

/// @brief FD 事件回调
/// @param fd 文件描述符
/// @param events 事件
/// @param context 上下文
/// @return 继续监听返回 1，停止监听返回 0
/// @note 短轮询间隔时触发
int UartClient::onFdEvent(int fd, int events, void* context) {
    UartClient* client = static_cast<UartClient*>(context);
    if (client == nullptr) {
        return 0;
    }
    return client->handleFdEvent(fd, events);
}

/// @brief 处理 FD 事件
/// @param fd 文件描述符
/// @param events 事件
/// @return 继续监听返回 1，停止监听返回 0
int UartClient::handleFdEvent(int fd, int events) {
    if (!isConnected() || fd != mFd) {
        return 0;
    }

    if ((events & (cdroid::Looper::EVENT_ERROR
        | cdroid::Looper::EVENT_HANGUP)) != 0) {
        int error = EIO;
        if ((events & cdroid::Looper::EVENT_HANGUP) != 0) {
            error = ENODEV;
        }
        disconnectOnDeviceError(error, events);
        return 0;
    }

    if ((events & cdroid::Looper::EVENT_INPUT) != 0) {
        readAvailable();
    }
    return isConnected() ? 1 : 0;
}

/// @brief 启动读取驱动
/// @return true: 成功, false: 失败
bool UartClient::startReadDriver() {
    if (!mUseFdMode) {
        startTick(0);
        return true;
    }

    mLooper = cdroid::Looper::getForThread();
    if (mLooper == nullptr) {
        LOGE("UartClient fd mode failed: current looper is null");
        return false;
    }

    const int events = cdroid::Looper::EVENT_INPUT
        | cdroid::Looper::EVENT_ERROR
        | cdroid::Looper::EVENT_HANGUP;
    if (mLooper->addFd(mFd, 0, events, onFdEvent, this) < 0) {
        LOGE("UartClient addFd failed. device=%s fd=%d", mConfig.device.c_str(), mFd);
        mLooper = nullptr;
        return false;
    }
    mFdRegistered = true;
    return true;
}

/// @brief 停止读取驱动
void UartClient::stopReadDriver() {
    if (mFdRegistered && mLooper != nullptr && mFd >= 0) {
        mLooper->removeFd(mFd);
    }
    mFdRegistered = false;
    mLooper = nullptr;
    stopTick();
}

/// @brief 读取数据
void UartClient::readAvailable() {
    if (!isConnected()) {
        return;
    }

    std::vector<uint8_t> buffer(mConfig.readBufferSize > 0 ? mConfig.readBufferSize : 4096);
    for (;;) {
        ssize_t n = read(mFd, buffer.data(), buffer.size());
        if (n > 0) {
            Event ev;
            ev.type = Event::DATA;
            ev.data.assign(buffer.begin(), buffer.begin() + n);
            dispatchEvent(ev);
            if (!isConnected()) return;
            if (static_cast<size_t>(n) < buffer.size()) {
                break;
            }
            continue;
        }

        if (n == 0 || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            break;
        }

        Event ev;
        ev.type = Event::ERROR;
        ev.error = errno;
        if (errno == EIO || errno == ENODEV || errno == EBADF) {
            disconnectOnDeviceError(errno, 0);
            return;
        }
        dispatchEvent(ev);
        break;
    }
}

/// @brief 打开 FD 设备
/// @return 文件描述符
int UartClient::openDevice() {
    int fd = open(mConfig.device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        LOGE("UartClient open failed. device=%s errno=%d err=%s",
            mConfig.device.c_str(), errno, strerror(errno));
    }
    return fd;
}

/// @brief 配置设备
/// @param fd 文件描述符
/// @return 0 成功；-1 获取属性失败；-2 波特率无效；-3 数据位无效；-4 校验位无效
/// @return -5 停止位无效；-6 平台不支持硬件流控；-7 流控模式无效；-8 应用属性失败
int UartClient::configureDevice(int fd) {
    termios options;
    if (tcgetattr(fd, &options) != 0) {
        LOGE("UartClient tcgetattr failed. errno=%d err=%s", errno, strerror(errno));
        return -1;
    }

    const speed_t baud = baudToTermios(mConfig.baudRate);
    if (baud == static_cast<speed_t>(0)) {
        LOGE("UartClient unsupported baud rate. baud=%d", mConfig.baudRate);
        return -2;
    }

    cfmakeraw(&options);
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    switch (mConfig.dataBits) {
    case 5: options.c_cflag |= CS5; break;
    case 6: options.c_cflag |= CS6; break;
    case 7: options.c_cflag |= CS7; break;
    case 8: options.c_cflag |= CS8; break;
    default:
        LOGE("UartClient unsupported data bits. dataBits=%d", mConfig.dataBits);
        return -3;
    }

    switch (mConfig.parity) {
    case 'N':
    case 'n':
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'O':
    case 'o':
        options.c_cflag |= PARENB;
        options.c_cflag |= PARODD;
        options.c_iflag |= INPCK;
        break;
    case 'E':
    case 'e':
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
        break;
    default:
        LOGE("UartClient unsupported parity. parity=%c", mConfig.parity);
        return -4;
    }

    if (mConfig.stopBits == 1) {
        options.c_cflag &= ~CSTOPB;
    } else if (mConfig.stopBits == 2) {
        options.c_cflag |= CSTOPB;
    } else {
        LOGE("UartClient unsupported stop bits. stopBits=%d", mConfig.stopBits);
        return -5;
    }

    options.c_iflag &= ~(IXON | IXOFF | IXANY);
#ifdef CRTSCTS
    options.c_cflag &= ~CRTSCTS;
#endif
    if (mConfig.flowControl == 1) {
#ifdef CRTSCTS
        options.c_cflag |= CRTSCTS;
#else
        LOGE("UartClient hardware flow control is unsupported on this platform");
        return -6;
#endif
    } else if (mConfig.flowControl == 2) {
        options.c_iflag |= (IXON | IXOFF | IXANY);
    } else if (mConfig.flowControl != 0) {
        LOGE("UartClient unsupported flow control. flow=%d", mConfig.flowControl);
        return -7;
    }

    options.c_cc[VTIME] = 0;
    options.c_cc[VMIN] = 0;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        LOGE("UartClient tcsetattr failed. errno=%d err=%s", errno, strerror(errno));
        return -8;
    }

    return 0;
}

/// @brief 释放设备占用
void UartClient::releaseDeviceClaim() {
    if (!mDeviceClaimed) {
        return;
    }

    std::lock_guard<std::mutex> lock(sDeviceLock);
    sUsedDevices.erase(mConfig.device);
    mDeviceClaimed = false;
}

/// @brief 故障时断开连接
/// @param error 错误码
/// @param events 事件
void UartClient::disconnectOnDeviceError(int error, int events) {
    const bool notifyDisconnected = mRunning;
    stopReadDriver();
    mRunning = false;

    LOGE("UartClient device error. device=%s errno=%d err=%s events=0x%x",
        mConfig.device.c_str(), error, strerror(error), events);

    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
    releaseDeviceClaim();

    Event errorEvent;
    errorEvent.type = Event::ERROR;
    errorEvent.error = error;
    dispatchEvent(errorEvent);

    if (notifyDisconnected) {
        Event disconnected;
        disconnected.type = Event::DISCONNECTED;
        dispatchEvent(disconnected);
    }
}

/// @brief 写入数据
/// @param data 数据
/// @param len 数据长度
/// @return 写入的字节数
ssize_t UartClient::writeAll(const uint8_t* data, size_t len) {
    size_t written = 0;
    const int64_t startMs = cdroid::SystemClock::uptimeMillis();

    while (written < len) {
        ssize_t n = write(mFd, data + written, len - written);
        if (n > 0) {
            written += static_cast<size_t>(n);
            continue;
        }

        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            const int64_t nowMs = cdroid::SystemClock::uptimeMillis();
            if (mConfig.writeTimeoutMs >= 0 && nowMs - startMs >= mConfig.writeTimeoutMs) {
                break;
            }

            pollfd pfd;
            pfd.fd = mFd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            poll(&pfd, 1, 10);
            continue;
        }

        Event ev;
        ev.type = Event::ERROR;
        ev.error = errno;
        dispatchEvent(ev);
        break;
    }

    return static_cast<ssize_t>(written);
}
