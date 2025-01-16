
#ifndef __packet_buffer_h__
#define __packet_buffer_h__

#include "ipacket_buffer.h"
#include "mcu_ui.h"

class SDHWPacketBuffer : public IPacketBuffer {
public:
    SDHWPacketBuffer();
    ~SDHWPacketBuffer();
    // static SDHWPacketBuffer *instance() {
    //     static SDHWPacketBuffer s_ins;
    //     return &s_ins;
    // }

    /// @brief 设置RCV包的类型以及命令字
    /// @param type 类型,区分来自哪个串口
    /// @param cmd 命令字,区分回调时调用哪个函数
    virtual void        setType(BufferType type, BufferType cmd);

protected:

protected:
    virtual BuffData   *obtain(BufferType type);                    // 创建(接收消息用)
    virtual BuffData   *obtain(BufferType type, uint16_t datalen);    // 创建(发送消息用)
    virtual void        recycle(BuffData *buf);                     // 回收
    virtual int         add(BuffData *buf, uint8_t *in_buf, int len); // 添加数据
    virtual bool        complete(BuffData *buf);                    // 数据完整
    virtual bool        compare(BuffData *src, BuffData *dst);      // 对比数据
    virtual bool        check(BuffData *buf);                       // 校验数据
    virtual std::string str(BuffData *buf);                         // 格式化字符串
    virtual void        check_code(BuffData *buf);                  // 生成校验码
    virtual IAck       *ack(BuffData *bf);                          // 转化成应答包
private:
    std::list<BuffData *> mBuffs;
    UI2MCU                mSND;
    MCU2UI                mRCV;
};

#endif
