#include "co_tcp_session_recv.hpp"
#include "co_tcp_accept_conn.hpp"
#include "utils/logger.hpp"
#include "utils/co_pub.hpp"
#include "co_tcp_pub.hpp"
#include <stdio.h>

namespace cpp_streamer
{
CoTcpSessionRecvPromise::CoTcpSessionRecvPromise(TcpCoAcceptConn* accept_conn, Logger* logger)
    : logger_(logger), accept_conn_(accept_conn)
{
}

CoTcpSessionRecvPromise::~CoTcpSessionRecvPromise()
{
}

bool CoTcpSessionRecvPromise::await_ready() const noexcept {
    return false;
}

void CoTcpSessionRecvPromise::await_suspend(std::coroutine_handle<> h) noexcept {
    handle_ = h;

    //if buffer has data, we can return immediately
    if (accept_conn_->recv_buffer_.DataLen() > 0) {
        HandleReceive(accept_conn_->recv_buffer_.DataLen());
        return;
    }

    accept_conn_->AddRecvAwaiter(this);
}

RecvResult CoTcpSessionRecvPromise::await_resume() const noexcept {
    return RecvResult{current_status_, current_recv_len_};
}

void CoTcpSessionRecvPromise::HandleReceive(ssize_t nread) {
    if (nread <= 0) {
        LogErrorf(logger_, "CoTcpSessionRecvPromise OnReceive error: %zd, %s", nread, uv_strerror((int)nread));
        current_status_ = RECV_ERROR;
        handle_.resume();
        return;
    }
    if (nread > static_cast<ssize_t>(accept_conn_->recv_size_)) {
        nread = static_cast<ssize_t>(accept_conn_->recv_size_);
        memcpy(accept_conn_->recv_data_, accept_conn_->recv_buffer_.Data(), nread);
        accept_conn_->recv_buffer_.ConsumeData((int)nread);
        current_status_ = RECV_OK;
        current_recv_len_ = (int)nread;
        handle_.resume();
        return;
    }
    memcpy(accept_conn_->recv_data_, accept_conn_->recv_buffer_.Data(), nread);
    accept_conn_->recv_buffer_.ConsumeData((int)nread);
    current_status_ = RECV_OK;
    current_recv_len_ = (int)nread;
    handle_.resume();
}

}