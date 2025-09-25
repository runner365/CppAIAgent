#ifndef CO_TCP_SERVER_H
#define CO_TCP_SERVER_H
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // ÆÁ±Î Windows ¾É°æÈßÓàÍ·ÎÄ¼þ£¨°üÀ¨ winsock.h£©
#endif
#include "utils/ipaddress.hpp"
#include "utils/logger.hpp"
#include "co_tcp_pub.hpp"

#include <memory>
#include <string>
#include <queue>
#include <coroutine>
#include <uv.h>

namespace cpp_streamer
{

void on_uv_connection(uv_stream_t* handle, int status);

class CoTcpServer;
class TcpCoAcceptConn;

class CoTcpServerPromise
{
friend class CoTcpServer;
public:
    CoTcpServerPromise(CoTcpServer* server,
                Logger* logger);
    ~CoTcpServerPromise();
    
public://for co_await
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    std::shared_ptr<TcpCoAcceptConn> await_resume() const noexcept;

private:
    void AcceptConnection(int status, uv_stream_t* handle);

private:
    std::coroutine_handle<> handle_;

private:
    Logger* logger_ = nullptr;
    CoTcpServer* server_ = nullptr;

private:
    TCP_CONNECT_STATUS current_status_ = TCP_CONNECT_PENDING;
    uv_stream_t* current_handle_ = nullptr;
};

class CoTcpServer
{
friend void on_uv_connection(uv_stream_t* handle, int status);
friend class CoTcpServerPromise;
public:
    CoTcpServer(uv_loop_t* loop,
                const std::string& host,
                int port,
                Logger* logger);
    ~CoTcpServer();

public:
    CoTcpServerPromise CoAccept();

private:
    void AddAwaiter(CoTcpServerPromise* awaiter);

private:
    void OnConnection(int status, uv_stream_t* handle);

public:
    uv_loop_t* loop_ = nullptr;
    std::string host_;
    uint16_t port_ = 0;
    Logger* logger_ = nullptr;
    bool ssl_enable = false;

public:
    struct sockaddr_in server_addr_;
    uv_tcp_t server_handle_;

private:
    std::queue<CoTcpServerPromise*> awaiters_;
};

}

#endif