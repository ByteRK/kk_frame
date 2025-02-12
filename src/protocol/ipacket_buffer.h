
#ifndef __ipacket_buffer_h__
#define __ipacket_buffer_h__

#include "common.h"

typedef enum {
    BT_NULL = 0,
    BT_MCU,       // MCU数据
    BT_BTN,       // 按键数据
    BT_TUYA,      // 涂鸦数据
} BufferType;

#pragma pack(1)
typedef struct {
    short type;   // 数据类型
    short slen;   // 分配缓冲区大小
    short len;    // 有效数据长度
    uint8_t buf[1]; // 缓冲区
} BuffData;
#pragma pack()

class IAck;
class IPacketBuffer {
public:
    virtual void        setType(BufferType type,BufferType cmd)    = 0; // 配置ack包属性
    virtual BuffData   *obtain(BufferType type)                    = 0; // 创建
    virtual BuffData   *obtain(BufferType type, uint16_t datalen)    = 0; // 创建
    virtual void        recycle(BuffData *buf)                     = 0; // 回收
    virtual int         add(BuffData *buf, uint8_t *in_buf, int len) = 0; // 添加数据
    virtual bool        complete(BuffData *buf)                    = 0; // 数据完整
    virtual bool        compare(BuffData *src, BuffData *dst)      = 0; // 对比数据
    virtual bool        check(BuffData *buf)                       = 0; // 校验数据
    virtual std::string str(BuffData *buf)                         = 0; // 格式化字符串
    virtual void        check_code(BuffData *buf)                  = 0; // 生成校验码
    virtual IAck       *ack(BuffData *bf)                          = 0; // 转化成ack
};

// 数据流出 UI -->
class IAsk {
public:
    virtual int getCMD() = 0;
public:
    BuffData *mBf;
};

// 数据流入 UI <--
class IAck {
public:
    virtual int  getType() = 0;                                //
    virtual int  getCMD()  = 0;                                //
    virtual int  getData(int pos) = 0;                         //
    virtual int  getData2(int pos, bool swap = false) = 0;     //
public:
    short  mDlen;
    short *mPlen;
    uint8_t *mBuf;
};

#endif
