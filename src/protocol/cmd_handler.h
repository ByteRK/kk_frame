
#ifndef __cmd_handler_h__
#define __cmd_handler_h__

#include "ipacket_buffer.h"

///////////////////////////////////////////////////////////////////////////

#define __make_short(a, b) (uint16_t(a) << 8 | (b))
#define __bind_hander(F) std::bind(F, this, std::placeholders::_1)

typedef std::function<void(IAck *)> cmd_callback;

class IHandler {
public:
    virtual void onRecv(IAck *ack) {
        auto it = mCMDHandlers.find(ack->getCMD());
        if (it == mCMDHandlers.end()) {
            onCommDeal(ack);
            return;
        }

        it->second(ack);
    }
    virtual void regHandler(int cmd, cmd_callback h) {
        auto it = mCMDHandlers.find(cmd);
        if (it != mCMDHandlers.end()) {
            LOGE("cmd handler already reg. cmd=%x", cmd);
            return;
        }

        LOGD("cmd handler reg. cmd=%x handle=%p", cmd, h);
        mCMDHandlers[cmd] = h;
    }
    virtual void onCommDeal(IAck *ack){
        LOGE("cmd not deal. cmd=%x", ack->getCMD());
    }

protected:
    std::map<int, cmd_callback> mCMDHandlers; // 协议处理接口
};

///////////////////////////////////////////////////////////////////////////////

class CHandlerManager {
    typedef std::vector<IHandler *> handlers;
public:
    static CHandlerManager *ins() {
        static CHandlerManager stIns;
        return &stIns;
    }

    // 需要立即响应的处理
    bool addHandler(int cmd, IHandler *hd);
    void removeHandler(IHandler *hd);
    // 消息回调
    void onCommand(IAck *ack);
    // 帧事件
    void onTick();

protected:
    CHandlerManager();

private:
    int64_t                   mLastRecvTime;
    int                       mRecvTimeout;
    std::map<int, handlers>   mHandlers;
};

extern CHandlerManager *g_objHandler;

#endif
