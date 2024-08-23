
#ifndef __uart_client_h__
#define __uart_client_h__

#include "proto.h"
#include "packet_buffer.h"
#include <socket_client.h>
#include <socket_server.h>
#include <uart2socket.h>

// 串口通信
class UartClient : public SocketClient {
public:
    UartClient(IPacketBuffer *ipacket, BufferType type, UartOpenReq &uartInfo
        , const std::string &ip, short port, int recv_space);
    ~UartClient();

    virtual int  init();
    virtual void onTick();
    bool isOk();

    int send(BuffData *ask);

protected:    
    virtual int  readData();
    virtual int  onRecvData();
    virtual void onStatusChange();
    virtual bool isTimeout(int out_time = 0);
    virtual int  checkEvents();
    virtual int  handleEvents();
    virtual int  getRecvSpace();

    int  onUartData(uchar *buf, int len);
    void sendTrans(BuffData *ask);
    void sendConn();
    void sendHeart();

protected:
    IPacketBuffer        *mPacketBuff;   // 数据包处理器
    BufferType            mBufType;      // 缓存类型
    UartOpenReq           mUartInfo;     // 串口连接信息
    std::string           mIp;
    short                 mPort;
    std::list<BuffData *> mSendQueue;    // 发包队列
    BuffData             *mLastRecv;     // 上次接收数据包
    BuffData             *mCurrRecv;     // 本次接收的数据包
    int64_t               mLastRecvTime; // 最后一次收包时间
    int64_t               mLastSendTime; // 最后一次发包时间
    int64_t               mLastSndHeart;
    int                   mChkErrCount;  // 错误次数
    int64_t               mSendCount;    // 发包个数
    int64_t               mRecvCount;    // 收包个数
    int                   mRecvSpace;  // 发包后接收间隔时间（毫秒）
    uchar                 mSerialOk : 1;
};

#endif
