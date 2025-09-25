#ifndef CO_TCP_ACCEPT_CONN_HPP
#define CO_TCP_ACCEPT_CONN_HPP
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // ÆÁ±Î Windows ¾É°æÈßÓàÍ·ÎÄ¼þ£¨°üÀ¨ winsock.h£©
#endif
#include "co_tcp_pub.hpp"
#include "co_tcp_session_recv.hpp"
#include "co_tcp_session_send.hpp"
#include "utils/logger.hpp"
#include "utils/data_buffer.hpp"

#include <memory>
#include <string>
#include <queue>
#include <coroutine>
#include <uv.h>

namespace cpp_streamer
{
//for server accept
class TcpCoAcceptConn
{
friend class CoTcpSessionRecvPromise;
friend class CoTcpSessionSendPromise;
public:
    TcpCoAcceptConn(uv_stream_t* handle, uv_tcp_t* uv_tcp_handle, TCP_CONNECT_STATUS status, Logger* logger);
    ~TcpCoAcceptConn();

public:
    std::string GetRemoteEndpoint() const;
    std::string GetLocalEndpoint() const;

public:
    CoTcpSessionRecvPromise CoReceive(uint8_t* recv_data, size_t recv_size);
    CoTcpSessionSendPromise CoSend(const uint8_t* data, size_t data_size);
    void Close();

private:
    void AddRecvAwaiter(CoTcpSessionRecvPromise* awaiter);
    void AddSendAwaiter(CoTcpSessionSendPromise* awaiter);

private:
    void OnAlloc(uv_buf_t* buf);
    void OnReceive(ssize_t nread, const uv_buf_t* buf);
    void OnWrite(write_req_t* req, int status);

private:
    static void OnUvWrite(uv_write_t* req, int status);
    static void OnUvAlloc(uv_handle_t* handle,
        size_t suggested_size,
        uv_buf_t* buf);
    static void OnUvRead(uv_stream_t* handle,
        ssize_t nread,
        const uv_buf_t* buf);
private:
    int SendData(const uint8_t* data, size_t size);

public:
    Logger* logger_ = nullptr;
    uv_stream_t* handle_ = nullptr;
    uv_tcp_t* uv_tcp_handle_ = nullptr;
    TCP_CONNECT_STATUS status_ = TCP_CONNECT_PENDING;
    bool closed_ = false;

private:
    uint8_t uv_recv_data_[20 * 1024] = {0}; // 20K read buffer
    uint8_t* recv_data_ = nullptr;
    size_t recv_size_ = 0;

private:
    DataBuffer recv_buffer_;

private:
    std::queue<CoTcpSessionRecvPromise*> read_awaiters_;
    std::queue<CoTcpSessionSendPromise*> write_awaiters_;
};

}

#endif