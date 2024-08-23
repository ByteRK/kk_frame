#ifndef __socket_client_h__
#define __socket_client_h__

#include "client.h"

// tcp client
class SocketClient : public Client {
public:
    SocketClient();
    ~SocketClient();

    virtual int  init(const char *ip = 0, ushort port = 0);
    virtual void onTick();

protected:
    virtual int  onRecvData();
    virtual void onStatusChange();

    bool         connectServer();
    bool         onCheckConnecting();

protected:
    bool        mInit;
    std::string mIp;
    ushort      mPort;
    ushort      mRPort;
    int64_t     mLastConnTime;
    int         mSockId;
};

#endif
