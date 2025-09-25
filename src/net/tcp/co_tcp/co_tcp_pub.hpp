#ifndef CO_TCP_PUB_HPP
#define CO_TCP_PUB_HPP
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // ÆÁ±Î Windows ¾É°æÈßÓàÍ·ÎÄ¼þ£¨°üÀ¨ winsock.h£©
#endif
#include "utils/logger.hpp"
#include <memory>
#include <string>
#include <queue>
#include <coroutine>
#include <uv.h>

namespace cpp_streamer
{
typedef enum {
    TCP_CONNECT_PENDING = 0,
    TCP_CONNECT_SUCCESS = 1,
    TCP_CONNECT_FAILED = -1,
} TCP_CONNECT_STATUS;

typedef enum {
    RECV_PENDING = 0,
    RECV_OK = 1,
    RECV_ERROR = -1,
} RECV_STATUS;

class RecvResult
{
public:
    RECV_STATUS status = RECV_PENDING;
    int recv_len = 0;
};

typedef enum {
    SEND_PENDING = 0,
    SEND_OK = 1,
    SEND_ERROR = -1,
} SEND_STATUS;

class SendResult
{
public:
    SEND_STATUS status = SEND_PENDING;
    int sent_len = 0;
};

void OnCoConnUVClose(uv_handle_t *handle);

//for TCP connection awaiter callback interface in client side
class CoTcpConnectAwaiterCallbackI
{
public:
    virtual void OnAwaiterConnect(int status) = 0;
};

class CoTcpSendAwaiterCallbackI
{
public:
    virtual void OnAwaiterSend(int sent_len) = 0;
};

class CoTcpRecvAwaiterCallbackI
{
public:
    virtual void OnAwaiterRecv(int recv_len) = 0;
};

}

#endif