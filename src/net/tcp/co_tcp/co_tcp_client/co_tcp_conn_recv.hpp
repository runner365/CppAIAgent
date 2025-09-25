#ifndef CO_TCP_CONN_RECV_H
#define CO_TCP_CONN_RECV_H
#include "net/tcp/tcp_pub.hpp"
#include "net/tcp/co_tcp/co_tcp_pub.hpp"
#include "utils/logger.hpp"
#include <coroutine>
#include <uv.h>

namespace cpp_streamer
{

void OnUVClientAlloc(uv_handle_t* handle,
                    size_t suggested_size,
                    uv_buf_t* buf);

void OnUVClientRead(uv_stream_t* handle,
                    ssize_t nread,
                    const uv_buf_t* buf);

class TcpCoConn;
class CoTcpRecvPromise : public CoTcpRecvAwaiterCallbackI
{
public:
    CoTcpRecvPromise(TcpCoConn* co_conn, Logger* logger);
    virtual ~CoTcpRecvPromise();

public://for co_await
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    RecvResult await_resume() const noexcept;

public:
    virtual void OnAwaiterRecv(int recv_len) override;

private:
    TcpCoConn* co_conn_ = nullptr;
    Logger* logger_ = nullptr;

private:
    std::coroutine_handle<> handler_;
    RecvResult result_;
};

}
#endif