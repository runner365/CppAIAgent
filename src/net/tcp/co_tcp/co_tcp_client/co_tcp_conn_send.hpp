#ifndef CO_TCP_CONN_SEND_H
#define CO_TCP_CONN_SEND_H
#include "net/tcp/tcp_pub.hpp"
#include "net/tcp/co_tcp/co_tcp_pub.hpp"
#include "utils/logger.hpp"

#include <memory>
#include <coroutine>
#include <uv.h>

namespace cpp_streamer
{
void OnUVClientWrite(uv_write_t* req, int status);

class TcpCoConn;
class CoTcpSendPromise : public CoTcpSendAwaiterCallbackI
{
friend void OnUVClientWrite(uv_write_t* req, int status);

public:
    CoTcpSendPromise(TcpCoConn* co_conn, Logger* logger);
    virtual ~CoTcpSendPromise();

public://for co_await
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    SendResult await_resume() const noexcept;

public:
    virtual void OnAwaiterSend(int sent_len) override;

private:
    TcpCoConn* co_conn_ = nullptr;
    Logger* logger_ = nullptr;
    std::coroutine_handle<> handler_;

private:
    SendResult result_;
};

}

#endif