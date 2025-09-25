#include "co_tcp_conn_recv.hpp"
#include "co_tcp_conn.hpp"
#include <uv.h>

namespace cpp_streamer
{
CoTcpRecvPromise::CoTcpRecvPromise(TcpCoConn* co_conn, Logger* logger):co_conn_(co_conn), logger_(logger)
{
    co_conn_->SetRecvAwaiterCallback(this);
}

CoTcpRecvPromise::~CoTcpRecvPromise() {
    co_conn_->SetRecvAwaiterCallback(nullptr);
}

bool CoTcpRecvPromise::await_ready() const noexcept {
    return false;
}

void CoTcpRecvPromise::await_suspend(std::coroutine_handle<> h) noexcept {
    handler_ = h;
    // if buffer has data, we can return immediately
    if (co_conn_->recv_buffer_.DataLen() > 0) {
        OnAwaiterRecv((int)co_conn_->recv_buffer_.DataLen());
        return;
    }
    co_conn_->StartReceive();
}

RecvResult CoTcpRecvPromise::await_resume() const noexcept {
    return result_;
}

void CoTcpRecvPromise::OnAwaiterRecv(int recv_len) {
    if (recv_len <= 0) {
        result_.status = RECV_ERROR;
        handler_.resume();
        LogErrorf(logger_, "CoTcpRecvPromise OnRead error, recv_len:%d", recv_len);
        return;
    }
    if (recv_len > static_cast<int>(co_conn_->recv_size_)) {
        recv_len = static_cast<int>(co_conn_->recv_size_);
        memcpy(co_conn_->recv_data_, co_conn_->recv_buffer_.Data(), recv_len);
        co_conn_->recv_buffer_.ConsumeData(recv_len);
        handler_.resume();
        result_.status = RECV_OK;
        result_.recv_len = recv_len;
        return;
    }
    memcpy(co_conn_->recv_data_, co_conn_->recv_buffer_.Data(), recv_len);
    co_conn_->recv_buffer_.ConsumeData(recv_len);
    result_.status = RECV_OK;
    result_.recv_len = recv_len;
    handler_.resume();
}

}