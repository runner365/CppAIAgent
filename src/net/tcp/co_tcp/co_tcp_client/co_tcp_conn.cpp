#include "co_tcp_conn.hpp"
#include "co_tcp_conn_send.hpp"
#include "co_tcp_conn_recv.hpp"
#include "utils/ipaddress.hpp"
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // 屏蔽 Windows 旧版冗余头文件（包括 winsock.h）
#include <windows.h>  // 如果需要 Windows 基础定义
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif
#include <uv.h>
#include <stdio.h>

namespace cpp_streamer
{

CoTcpConnectPromise::CoTcpConnectPromise(TcpCoConn* co_conn, Logger* logger):
    co_conn_(co_conn), logger_(logger)
{
    co_conn_->SetConnectAwaiterCallback(this);
}

CoTcpConnectPromise::~CoTcpConnectPromise()
{
    LogDebugf(logger_, "CoTcpConnectPromise destructor called..");
}

bool CoTcpConnectPromise::await_ready() const noexcept {
    return false;
}

void CoTcpConnectPromise::await_suspend(std::coroutine_handle<> h) noexcept {
    handle_ = h;
    int r = co_conn_->StartConnect();
    if (r < 0) {
        status_ = TCP_CONNECT_FAILED;
        handle_.resume();
    }
}

TCP_CONNECT_STATUS CoTcpConnectPromise::await_resume() const noexcept {
    return status_;
}

void CoTcpConnectPromise::OnAwaiterConnect(int status) {
    if (status == 0) {
        status_ = TCP_CONNECT_SUCCESS;
    } else {
        status_ = TCP_CONNECT_FAILED;
    }
    handle_.resume();
}

TcpCoConn::TcpCoConn(uv_loop_t* loop, const std::string& host, uint16_t dst_port, bool ssl_enable, Logger* logger):
    loop_(loop), host_(host), port_(dst_port), ssl_enable_(ssl_enable), logger_(logger)
{
    client_  = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, client_);
}

TcpCoConn::~TcpCoConn()
{
    if (connect_) {
        uv_read_stop(connect_->handle);
        free(connect_);
        connect_ = nullptr;
    }
    if (client_) {
        uv_close((uv_handle_t*)client_, OnCoConnUVClose);
        client_ = nullptr;
    }
    status_ = TCP_CONNECT_PENDING;
    client_ = nullptr;
    connect_ = nullptr;
}

CoTcpConnectPromise TcpCoConn::CoConnect() {
    return CoTcpConnectPromise(this, logger_);
}

CoTcpSendPromise TcpCoConn::CoSend(const uint8_t* data, size_t data_size) {
    send_buffer_.resize(data_size);
    memcpy(send_buffer_.data(), data, data_size);

    return CoTcpSendPromise(this, logger_);
}

CoTcpRecvPromise TcpCoConn::CoReceive(uint8_t* recv_data, size_t recv_size) {
    recv_data_ = recv_data;
    recv_size_ = recv_size;

    return CoTcpRecvPromise(this, logger_);
}

int TcpCoConn::StartConnect() {
    int r = 0;
    char port_sz[80];
    std::string dst_ip;
    struct sockaddr_in dst_addr;

    if (!IsIPv4(host_)) {
        snprintf(port_sz, sizeof(port_sz), "%d", port_);
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* ai  = NULL;
        if(getaddrinfo(host_.c_str(), port_sz, (const addrinfo*)&hints, &ai)) {
            LogErrorf(logger_, "getaddrinfo error for host:%s, port:%s", 
                    host_.c_str(), port_sz);
            connect_ = nullptr;
            return -1;
        }
        freeaddrinfo(ai);
        assert(sizeof(dst_addr) == ai->ai_addrlen);

        memcpy((void*)&dst_addr, ai->ai_addr, sizeof(dst_addr));
        dst_ip = GetIpStr(ai->ai_addr, port_);
    } else {
        GetIpv4Sockaddr(host_, htons(port_), (struct sockaddr*)&dst_addr);
        dst_ip = GetIpStr((sockaddr*)&dst_addr, port_);
    }
    connect_ = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    connect_->data = this;

    if ((r = uv_tcp_connect(connect_, client_,
                        (const struct sockaddr*)&dst_addr,
                        OnUVClientConnected)) != 0) {
        free(connect_);
        connect_ = nullptr;
        LogErrorf(logger_, "uv_tcp_connect error:%d", r);
        connect_ = nullptr;
        return -2;
    }

    return 0;
}

int TcpCoConn::StartSend() {
    size_t size = send_buffer_.size();
    char* new_data = (char*)send_buffer_.data();

    write_req_t* req = (write_req_t*)malloc(sizeof(write_req_t));
    req->buf = uv_buf_init(new_data, (unsigned int)size);

    connect_->handle->data = this;
    int ret = uv_write((uv_write_t*)req, connect_->handle, &req->buf, 1, OnUvWrite);
    if (ret) {
        LogErrorf(logger_, "TcpCoAcceptConn SendData uv_write error: %d, %s", ret, uv_strerror(ret));
        free(req);
        return -1;
    }
    return 0;
}

int TcpCoConn::StartReceive() {
    if (read_started_) {
        return 0;
    }
    int ret = 0;

    if ((ret = uv_read_start(connect_->handle, OnUVClientAlloc, OnUVClientRead)) != 0) {
        if (ret == UV_EALREADY) {
            LogInfof(logger_, "uv_read_start already started");
            return 0;
        }
        LogErrorf(logger_, "uv_read_start error: %d, %s", ret, uv_strerror(ret));
        return -1;
    }
    read_started_ = true;
    return 0;
}

void TcpCoConn::OnAlloc(uv_buf_t* buf) {
    buf->base = (char*)uv_read_buffer_;
    buf->len  = sizeof(uv_read_buffer_);
    return;
}

void TcpCoConn::OnRead(ssize_t nread, const uv_buf_t* buf) {
    if (nread <= 0) {
        LogErrorf(logger_, "TcpCoConn OnRead error: %zd, %s", nread, uv_strerror((int)nread));
        recv_awaiter_callback_->OnAwaiterRecv((int)nread);
        return;
    }

    recv_buffer_.AppendData(buf->base, nread);
    recv_awaiter_callback_->OnAwaiterRecv((int)nread);
}

void TcpCoConn::OnWrite(write_req_t* req, int status) {
    if (status == 0) {
        send_awaiter_callback_->OnAwaiterSend(req->buf.len);
    } else {
        LogErrorf(logger_, "TcpCoConn SendData failed: %d", status);
    }

    free(req);
}

void TcpCoConn::OnConnect(uv_connect_t *connect, int status) {
    if (status == 0) {
        status_ = TCP_CONNECT_SUCCESS;
    } else {
        status_ = TCP_CONNECT_FAILED;
        LogErrorf(logger_, "uv_tcp_connect failed, status: %d", status);
    }
    awaiter_callback_->OnAwaiterConnect(status);
}

void TcpCoConn::SetConnectAwaiterCallback(CoTcpConnectAwaiterCallbackI* callback) {
    awaiter_callback_ = callback;
}

void TcpCoConn::SetSendAwaiterCallback(CoTcpSendAwaiterCallbackI* callback) {
    send_awaiter_callback_ = callback;
}

void TcpCoConn::SetRecvAwaiterCallback(CoTcpRecvAwaiterCallbackI* callback) {
    recv_awaiter_callback_ = callback;
}

void TcpCoConn::OnUVClientConnected(uv_connect_t* conn, int status) {
    TcpCoConn* client = (TcpCoConn*)conn->data;
    if (client) {
        client->OnConnect(conn, status);
    }
}

void TcpCoConn::OnUvWrite(uv_write_t* req, int status) {
    TcpCoConn* conn = static_cast<TcpCoConn*>(req->handle->data);
    if (conn) {
        conn->OnWrite((write_req_t*)req, status);
    }
}

void TcpCoConn::OnUVClientAlloc(uv_handle_t* handle,
    size_t suggested_size,
    uv_buf_t* buf) {
    TcpCoConn* receiver = static_cast<TcpCoConn*>(handle->data);
    if (receiver) {
        receiver->OnAlloc(buf);
    }
}

void TcpCoConn::OnUVClientRead(uv_stream_t* handle,
    ssize_t nread,
    const uv_buf_t* buf)
{
    TcpCoConn* client = (TcpCoConn*)handle->data;
    if (client) {
        client->OnRead(nread, buf);
    }
    return;
}

}