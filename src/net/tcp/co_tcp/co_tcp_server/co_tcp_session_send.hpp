#ifndef CO_TCP_SESSION_SEND_HPP
#define CO_TCP_SESSION_SEND_HPP
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // ÆÁ±Î Windows ¾É°æÈßÓàÍ·ÎÄ¼þ£¨°üÀ¨ winsock.h£©
#endif
#include <memory>
#include <string>
#include <coroutine>
#include <uv.h>
#include "utils/logger.hpp"
#include "co_tcp_pub.hpp"
#include "net/tcp/tcp_pub.hpp"

namespace cpp_streamer
{

class TcpCoAcceptConn;
class CoTcpSessionSendPromise
{
friend class TcpCoAcceptConn;
public:
    CoTcpSessionSendPromise(TcpCoAcceptConn* accept_conn, const uint8_t* data, size_t size, Logger* logger);
    ~CoTcpSessionSendPromise();

public://for co_await
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    SendResult await_resume() const noexcept;

private:
    void HandleSend(int status, size_t sent_len);

private:
    TcpCoAcceptConn* accept_conn_ = nullptr;
    SendResult result_;
    std::coroutine_handle<> handler_;

private:
    Logger* logger_ = nullptr;
    //write_req_t* req_ = nullptr; // The write request to be sent

private:
    uint8_t* send_data_ = nullptr;
    size_t send_size_ = 0;
};

}

#endif