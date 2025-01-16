// 服务器，对接收和发送的数据广播给连接上来的客户端
#ifndef __socket_server_h__
#define __socket_server_h__

#include "client.h"

class SocketServer;

class ServerClient : public Client {
public:
    ServerClient(SocketServer *s, int fd);
    ~ServerClient();

protected:
    virtual int init();
    virtual void onTick();
    virtual int onRecvData();
    virtual void onStatusChange();

private:
    SocketServer  *mSvr;    
};

////////////////////////////////////////////////////////////////////////////////
class SocketServer : public Client {
public:
    SocketServer();
    ~SocketServer();

    virtual int init();
    virtual void onTick();

    virtual int sendData(const char *data, int len);
    virtual int sendData(const uint8_t *data, int len);
    void close(int fd);

protected:
    virtual int handleEvent(int fd, int events, void* data);
    virtual int onRecvData(){return 0;}

private:
    std::map<int, ServerClient*> mClients;
};

#endif
