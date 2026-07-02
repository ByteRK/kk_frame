/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-26
 * @FilePath: /kk_frame/src/comm/uart/uart_client.cc
 * @Description: Raw UART client
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
        default:
            return static_cast<speed_t>(0);
        }
    }

}

std::mutex UartClient::sDeviceLock;
std::set<std::string> UartClient::sUsedDevices;

UartClient::UartClient(const Config& config)
    : mConfig(config),
    mFd(-1),
    mRunning(false),
    mDeviceClaimed(false),
    mLastPollMs(0),
    mHandler(nullptr) { }

UartClient::~UartClient() {
    stop();
}

void UartClient::setHandler(TransportHandler* handler) {
    mHandler = handler;
}

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

bool UartClient::start() {
    if (mRunning) {
        return true;
    }

    if (mFd < 0 && init() != 0) {
        return false;
    }

    mRunning = true;
    mLastPollMs = cdroid::SystemClock::uptimeMillis();

    Event ev;
    ev.type = Event::CONNECTED;
    return sendEvent(ev) && mRunning;
}

void UartClient::stop() {
    const bool notifyDisconnected = mRunning;
    mRunning = false;
    cancelEventDispatch();

    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
    releaseDeviceClaim();

    if (notifyDisconnected) {
        Event ev;
        ev.type = Event::DISCONNECTED;
        sendEvent(ev);
    }
}

bool UartClient::isConnected() const {
    return mRunning && mFd >= 0;
}

void UartClient::onTick() {
    if (!isConnected()) {
        return;
    }

    const int64_t nowMs = cdroid::SystemClock::uptimeMillis();
    if (mConfig.pollIntervalMs > 0 && nowMs - mLastPollMs < mConfig.pollIntervalMs) {
        return;
    }
    mLastPollMs = nowMs;

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
            sendEvent(ev);
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

    std::vector<uint8_t> buffer(mConfig.readBufferSize > 0 ? mConfig.readBufferSize : 4096);
    for (;;) {
        ssize_t n = read(mFd, buffer.data(), buffer.size());
        if (n > 0) {
            Event ev;
            ev.type = Event::DATA;
            ev.data.assign(buffer.begin(), buffer.begin() + n);
            if (!sendEvent(ev)) {
                return;
            }
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
        sendEvent(ev);
        break;
    }
}

ssize_t UartClient::send(const uint8_t* data, size_t len, int /*id*/) {
    if (!isConnected() || data == nullptr || len == 0) {
        return -1;
    }

    return writeAll(data, len);
}

void UartClient::dispatchEvent(const Event& ev) {
    if (mHandler == nullptr) {
        return;
    }

    switch (ev.type) {
    case Event::CONNECTED:
        mHandler->onConnected();
        break;
    case Event::DISCONNECTED:
        mHandler->onDisconnected();
        break;
    case Event::DATA:
        if (!ev.data.empty()) {
            mHandler->onRecv(ev.data.data(), ev.data.size());
        }
        break;
    case Event::ERROR:
        mHandler->onError(ev.error);
        break;
    default:
        break;
    }
}

int UartClient::openDevice() {
    int fd = open(mConfig.device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        LOGE("UartClient open failed. device=%s errno=%d err=%s",
            mConfig.device.c_str(), errno, strerror(errno));
    }
    return fd;
}

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

void UartClient::releaseDeviceClaim() {
    if (!mDeviceClaimed) {
        return;
    }

    std::lock_guard<std::mutex> lock(sDeviceLock);
    sUsedDevices.erase(mConfig.device);
    mDeviceClaimed = false;
}

void UartClient::disconnectOnDeviceError(int error, short revents) {
    const bool notifyDisconnected = mRunning;
    mRunning = false;

    LOGE("UartClient device error. device=%s errno=%d err=%s revents=0x%x",
        mConfig.device.c_str(), error, strerror(error), revents);

    if (mFd >= 0) {
        close(mFd);
        mFd = -1;
    }
    releaseDeviceClaim();

    Event errorEvent;
    errorEvent.type = Event::ERROR;
    errorEvent.error = error;
    if (!sendEvent(errorEvent)) {
        return;
    }

    if (notifyDisconnected) {
        Event disconnected;
        disconnected.type = Event::DISCONNECTED;
        sendEvent(disconnected);
    }
}

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
        sendEvent(ev);
        break;
    }

    return static_cast<ssize_t>(written);
}
