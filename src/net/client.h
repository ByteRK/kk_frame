
#ifndef __client_h__
#define __client_h__

#include <arpa/inet.h>
#include <core/looper.h>
#include <core/systemclock.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/syscall.h> // for SYS_xxx definitions
#include <unistd.h>      // for syscall()

#include <comm_func.h>
#include <common.h>

#define LEN_4K 4096

using namespace cdroid;

typedef struct {
    uchar *start;
    uchar *pos;
    uchar *last;
    uchar *end;
    uchar  buf[1];
} SockBuf;

class Client : public LooperCallback, public EventHandler {
public:
    constexpr static int TIME_OUT   = 30000; // 超时时间
    constexpr static int HEART_TIME = 5000;  // 心跳间隔
public:
    typedef enum {
        ST_NULL = 0,
        ST_CONNECTING,
        ST_CONNECTED,
        ST_DISCONNECT,
    } STATUS;

public:
    Client();
    Client(int fd);
    ~Client();

    virtual int  init()   = 0;
    virtual void onTick() = 0;
    virtual bool isConn();
    virtual bool isAuthentication();
    virtual bool isTimeout(int out_time = 0);

    virtual int sendData(const char *data, int len);
    virtual int sendData(const uchar *data, int len);

protected:
    // 收包处理：0继续循环接收；!0中断本次接收
    virtual int onRecvData() = 0;

    virtual int  handleEvent(int fd, int events, void *data);
    virtual int  readData();
    virtual int  readFile();
    virtual void onStatusChange();

    // 自定义消息处理
    virtual int checkEvents();
    virtual int handleEvents();
    virtual int getRecvSpace();

    void initData();
    void setFd(int fd);
    void closeFd();

protected:
    int      mFd;
    int      mStatus;
    SockBuf *mRSBuf;       // 接收缓冲区
    bool     mIsAuth;      // 是否登陆
    int64_t  mLastRcvTime; // 最后收包时间
    int64_t  mLastSndTime; // 最后发包时间
};

#endif
