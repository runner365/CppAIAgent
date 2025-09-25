#include "co_tcp_accept_conn.hpp"
#include "utils/ipaddress.hpp"
#include "utils/logger.hpp"
#include <stdio.h>

namespace cpp_streamer
{

TcpCoAcceptConn::TcpCoAcceptConn(uv_stream_t* handle, uv_tcp_t* uv_tcp_handle, TCP_CONNECT_STATUS status, Logger* logger)
    : logger_(logger), handle_(handle), uv_tcp_handle_(uv_tcp_handle), status_(status), recv_buffer_(50*1024)
{
    uv_tcp_handle_->data = this;
    uv_stream_t* stream = reinterpret_cast<uv_stream_t*>(uv_tcp_handle_);

    uv_read_start(stream,
            static_cast<uv_alloc_cb>(OnUvAlloc),
            static_cast<uv_read_cb>(OnUvRead));
}

TcpCoAcceptConn::~TcpCoAcceptConn()
{
    Close();
}

CoTcpSessionRecvPromise TcpCoAcceptConn::CoReceive(uint8_t* recv_data, size_t recv_size) {
    recv_data_ = recv_data;
    recv_size_ = recv_size;
    return CoTcpSessionRecvPromise(this, logger_);
}

CoTcpSessionSendPromise TcpCoAcceptConn::CoSend(const uint8_t* data, size_t data_size) {
    return CoTcpSessionSendPromise(this, data, data_size, logger_);
}

void TcpCoAcceptConn::Close() {
    if (closed_) {
        return;
    }
    closed_ = true;
    uv_read_stop(reinterpret_cast<uv_stream_t*>(uv_tcp_handle_));

    uv_close(reinterpret_cast<uv_handle_t*>(uv_tcp_handle_), static_cast<uv_close_cb>(OnCoConnUVClose));

    status_ = TCP_CONNECT_PENDING;
    handle_ = nullptr;
    uv_tcp_handle_ = nullptr;
    LogInfof(logger_, "co tcp accept connection is closed...");
}

std::string TcpCoAcceptConn::GetRemoteEndpoint() const {
    std::stringstream ss;
    struct sockaddr peer_name;
    int namelen = sizeof(peer_name);
    uint16_t port = 0;

    uv_tcp_getpeername(uv_tcp_handle_, &peer_name, &namelen);

    std::string remoteip = GetIpStr(&peer_name, port);

    ss << remoteip << ":" << port;
    return ss.str();
}

std::string TcpCoAcceptConn::GetLocalEndpoint() const {
    std::stringstream ss;
    struct sockaddr local_name;
    int namelen = sizeof(local_name);
    uint16_t port = 0;

    uv_tcp_getsockname(uv_tcp_handle_, &local_name, &namelen);

    std::string localip = GetIpStr(&local_name, port);

    ss << localip << ":" << port;
    return ss.str();
}

void TcpCoAcceptConn::OnAlloc(uv_buf_t* buf) {
    buf->base = (char*)uv_recv_data_;
    buf->len  = sizeof(uv_recv_data_);
}

void TcpCoAcceptConn::OnReceive(ssize_t nread, const uv_buf_t* buf) {
    if (read_awaiters_.empty()) {
        if (nread <= 0) {
            LogErrorf(logger_, "OnReceive error: %zd, %s", nread, uv_strerror((int)nread));
            return;
        }
        std::string data_str(reinterpret_cast<const char*>(buf->base), nread);
        LogErrorf(logger_, "No read awaiters, nread: %zd, data: %s", nread, data_str.c_str());

        return;
    }
    if (nread > 0) {
        recv_buffer_.AppendData(buf->base, nread);
    }
    auto awaiter_it = read_awaiters_.front();
    read_awaiters_.pop();
    awaiter_it->HandleReceive(nread);
}

void TcpCoAcceptConn::OnWrite(write_req_t* req, int status) {
    if (write_awaiters_.empty()) {
        LogErrorf(logger_, "No write awaiters");
        free(req->buf.base);
        free(req);
        return;
    }
    auto awaiter_it = write_awaiters_.front();
    write_awaiters_.pop();
    awaiter_it->HandleSend(status, req->buf.len);
    
    free(req->buf.base);
    free(req);
}

int TcpCoAcceptConn::SendData(const uint8_t* data, size_t size) {
    char* new_data = (char*)malloc(size);
    memcpy(new_data, data, size);

    write_req_t* req = (write_req_t*)malloc(sizeof(write_req_t));
    req->buf = uv_buf_init(new_data, (unsigned int)size);

    uv_tcp_handle_->data = this;
    int ret = uv_write((uv_write_t*)req, reinterpret_cast<uv_stream_t*>(uv_tcp_handle_), &req->buf, 1, TcpCoAcceptConn::OnUvWrite);
    if (ret) {
        LogErrorf(logger_, "TcpCoAcceptConn SendData uv_write error: %d, %s", ret, uv_strerror(ret));
        free(req->buf.base);
        free(req);
        return -1;
    }
    return 0;
}

void TcpCoAcceptConn::AddRecvAwaiter(CoTcpSessionRecvPromise* awaiter) {
    read_awaiters_.push(awaiter);
}

void TcpCoAcceptConn::AddSendAwaiter(CoTcpSessionSendPromise* awaiter) {
    write_awaiters_.push(awaiter);
}

void TcpCoAcceptConn::OnUvWrite(uv_write_t* req, int status) {
    TcpCoAcceptConn* session = static_cast<TcpCoAcceptConn*>(req->handle->data);

    if (session) {
        session->OnWrite((write_req_t*)req, status);
    }
    return;
}
void TcpCoAcceptConn::OnUvAlloc(uv_handle_t* handle,
    size_t suggested_size,
    uv_buf_t* buf) {
    TcpCoAcceptConn* conn = (TcpCoAcceptConn*)handle->data;
    if (conn) {
        conn->OnAlloc(buf);
    }
}

void TcpCoAcceptConn::OnUvRead(uv_stream_t* handle,
    ssize_t nread,
    const uv_buf_t* buf) {
    TcpCoAcceptConn* conn = (TcpCoAcceptConn*)handle->data;
    if (conn) {
        if (nread == 0) {
            return;
        }
        conn->OnReceive(nread, buf);
    }
    return;
}
}