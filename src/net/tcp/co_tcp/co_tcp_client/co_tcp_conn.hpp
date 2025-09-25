#ifndef CO_TCP_CONN_H
#define CO_TCP_CONN_H
#include "utils/ipaddress.hpp"
#include "utils/logger.hpp"
#include "co_tcp_pub.hpp"
#include "net/tcp/tcp_pub.hpp"
#include <memory>
#include <string>
#include <coroutine>
#include <uv.h>

namespace cpp_streamer
{
//for client connection
class TcpCoConn;
class CoTcpSendPromise;
class CoTcpRecvPromise;

//coroutine awaiter for TCP connection in client side
class CoTcpConnectPromise : public CoTcpConnectAwaiterCallbackI
{
friend class TcpCoConn;
public:
    CoTcpConnectPromise(TcpCoConn* co_conn, Logger* logger);
    virtual ~CoTcpConnectPromise();

public://for co_await
    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h) noexcept;
    TCP_CONNECT_STATUS await_resume() const noexcept;

public:
    virtual void OnAwaiterConnect(int status) override;
    
private:
    TcpCoConn* co_conn_ = nullptr;
    Logger* logger_ = nullptr;
    TCP_CONNECT_STATUS status_ = TCP_CONNECT_PENDING;
    std::coroutine_handle<> handle_;
};

//for client connection
class TcpCoConn
{
friend class CoTcpConnectPromise;
friend class CoTcpSendPromise;
friend class CoTcpRecvPromise;

public:
    TcpCoConn(uv_loop_t* loop, const std::string& host, uint16_t dst_port, bool ssl_enable, Logger* logger);
    ~TcpCoConn();

    CoTcpConnectPromise CoConnect();
    CoTcpSendPromise CoSend(const uint8_t* data, size_t data_size);
    CoTcpRecvPromise CoReceive(uint8_t* recv_data, size_t recv_size);

private:
    int StartConnect();
    void OnConnect(uv_connect_t *connect, int status);

private:
    int StartSend();
    void OnWrite(write_req_t* req, int status);

private:
    int StartReceive();
    void OnAlloc(uv_buf_t* buf);
    void OnRead(ssize_t nread, const uv_buf_t* buf);

private:
    void SetConnectAwaiterCallback(CoTcpConnectAwaiterCallbackI* callback);
    void SetSendAwaiterCallback(CoTcpSendAwaiterCallbackI* callback);
    void SetRecvAwaiterCallback(CoTcpRecvAwaiterCallbackI* callback);

private:
    static void OnUVClientConnected(uv_connect_t* conn, int status);
    static void OnUvWrite(uv_write_t* req, int status);
    static void OnUVClientAlloc(uv_handle_t* handle,
        size_t suggested_size,
        uv_buf_t* buf);
    static void OnUVClientRead(uv_stream_t* handle,
        ssize_t nread,
        const uv_buf_t* buf);
private:
    uv_loop_t* loop_ = nullptr;
    std::string host_;
    uint16_t port_ = 0;
    bool ssl_enable_ = false;
    Logger* logger_ = nullptr;

private:
    CoTcpConnectAwaiterCallbackI* awaiter_callback_ = nullptr;
    CoTcpSendAwaiterCallbackI *send_awaiter_callback_ = nullptr;
    CoTcpRecvAwaiterCallbackI *recv_awaiter_callback_ = nullptr;

private:
    uv_connect_t* connect_ = nullptr;
    uv_tcp_t* client_ = nullptr;
    TCP_CONNECT_STATUS status_ = TCP_CONNECT_PENDING;

private:
    std::vector<uint8_t> send_buffer_;

private:
    uint8_t* recv_data_ = nullptr;
    size_t recv_size_ = 0;
    bool read_started_ = false;

private:
    uint8_t uv_read_buffer_[20 * 1024] = {0}; // 20K read buffer
    DataBuffer recv_buffer_;
};
}
#endif