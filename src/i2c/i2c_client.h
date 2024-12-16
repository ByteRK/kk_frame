
#ifndef __iic_client_h__
#define __iic_client_h__

#include "proto.h"
#include "packet_buffer.h"
#include "socket_client.h"
#include "socket_server.h"
#include "i2c_mcu.h"

#define LEN_4K 4096
#define LEN_12 12

typedef struct {
    uchar *start;
    uchar *pos;
    uchar *last;
    uchar *end;
    uchar  buf[1];
} IICBuf;

typedef struct {
    uint8_t  type;  // 类型
    uint8_t  bus;   // 总线地址
    uint8_t  addr;  // 从机地址
    uint8_t  reg;   // 寄存器地址
    uint8_t  len;   // 读长度
} I2COpenReq;

// I2C通信
class I2CClient : public SocketClient{
public:
    enum {
        I2C_TYPE_HW = 0,
        I2C_TYPE_SW,
    };
public:
    I2CClient(IPacketBuffer *ipacket, BufferType type, I2COpenReq &i2cInfo
        , const std::string &ip, short port, int recv_space);
    ~I2CClient();

    int  init();
    void onTick();
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

    int  onI2cData(uchar *buf, int len);
    void sendTrans(BuffData *ask);
    void sendHeart();
    bool checkDealData();
protected:
    IPacketBuffer        *mPacketBuff;     // 数据包处理器
    BufferType            mBufType;        // 缓存类型
    I2COpenReq            mI2cOpenReq;     // I2C连接信息
    std::string           mIp;
    short                 mPort;
    std::list<BuffData *> mSendQueue;    // 发包队列
    BuffData             *mLastRecv;     // 上次接收数据包
    BuffData             *mCurrRecv;     // 本次接收的数据包
    int64_t               mLastRecvTime; // 最后一次收包时间
    int64_t               mLastSendTime; // 最后一次发包时间
    int64_t               mLastSndHeart; // 最后一次发送心跳包时间
    int                   mChkErrCount;  // 错误次数
    int64_t               mSendCount;    // 发包个数
    int64_t               mRecvCount;    // 收包个数
    int                   mRecvSpace;    // 发包后接收间隔时间（毫秒）
    uchar                 mSerialOk : 1;

    IICBuf               *mRSBuf; 
    int64_t               mLastDealDataTime;  // 上一次处理数据的时间
};

#endif
