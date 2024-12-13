
#ifndef __iic_client_h__
#define __iic_client_h__

#include "proto.h"
#include "packet_buffer.h"
#include "comm_func.h"
#include <stdio.h>
#include <unistd.h> 
#include "i2c_mcu.h"

#include <core/looper.h>
using namespace cdroid;


// #define I2C_SLAVE_ADDR 0x50
// #define I2C_SLAVE_FW_VERSION_ADDR 0xA0
// #define I2C_SLAVE_CUSTOM_ADDR 0xB0

// 软件I2C地址
#define I2C_ADDR_A 0x50 // 左
#define I2C_ADDR_B 0x58 // 右

#define LEN_4K 4096
#define LEN_12 12
typedef struct {
    uchar *start;
    uchar *pos;
    uchar *last;
    uchar *end;
    uchar  buf[1];
} IICBuf;

// I2C通信
class I2CClient : public EventHandler{
public:
    I2CClient(IPacketBuffer *ipacket, BufferType type,  int recv_space);
    ~I2CClient();

    int  init();
    void onTick();

    int send(BuffData *ask);

protected:    
    int  readData();
    int  onRecvData();
    int  checkEvents();
    virtual int  handleEvents();
    virtual int  getRecvSpace();

    int  onUartData(uchar *buf, int len);
    void sendTrans(BuffData *ask);
    bool checkDealData();
protected:
    IPacketBuffer        *mPacketBuff;   // 数据包处理器
    BufferType            mBufType;      // 缓存类型
    std::string           mIp;
    short                 mPort;
    std::list<BuffData *> mSendQueue;    // 发包队列
    BuffData             *mLastRecv;     // 上次接收数据包
    BuffData             *mCurrRecv;     // 本次接收的数据包
    int64_t               mLastRecvTime; // 最后一次收包时间
    int64_t               mLastSendTime; // 最后一次发包时间
    int64_t               mLastSndHeart;
    uint8_t               read_data[9] = {0};//接收数据
    int                   mChkErrCount;  // 错误次数
    int64_t               mSendCount;    // 发包个数
    int64_t               mRecvCount;    // 收包个数
    int                   mRecvSpace;  // 发包后接收间隔时间（毫秒）
    uchar                 mSerialOk : 1;
    IICBuf               *mRSBuf; 
    int64_t               mLastDealDataTime;  // 上一次处理数据的时间
};

#endif
