#ifndef CO_TCP_SESSION_RECV_HPP
#define CO_TCP_SESSION_RECV_HPP
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // ÆÁ±Î Windows ¾É°æÈßÓàÍ·ÎÄ¼þ£¨°üÀ¨ winsock.h£©
#endif
#include <memory>
#include <string>
#include <coroutine>
#include <uv.h>
#include "utils/logger.hpp"
#include "co_tcp_pub.hpp"

namespace cpp_streamer
{
class TcpCoAcceptConn;
class CoTcpSessionRecvPromise
{
public:
    CoTcpSessionRecvPromise(TcpCoAcceptConn* accept_conn,Logger* logger);
    ~CoTcpSessionRecvPromise();

public://for co_await
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    RecvResult await_resume() const noexcept;

public:
    void HandleReceive(ssize_t nread);

private:
    Logger* logger_ = nullptr;

private:
    std::coroutine_handle<> handle_;

private:
    TcpCoAcceptConn* accept_conn_;

private:
    RECV_STATUS current_status_ = RECV_PENDING;
    int current_recv_len_ = 0;
};

}

#endif