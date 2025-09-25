#include "co_tcp_conn_send.hpp"
#include "co_tcp_conn.hpp"
#include <uv.h>

namespace cpp_streamer
{
CoTcpSendPromise::CoTcpSendPromise(TcpCoConn* co_conn, Logger* logger) :co_conn_(co_conn), logger_(logger)
{
    co_conn_->SetSendAwaiterCallback(this);
}

CoTcpSendPromise::~CoTcpSendPromise()
{
}

bool CoTcpSendPromise::await_ready() const noexcept {
    return false;
}

void CoTcpSendPromise::await_suspend(std::coroutine_handle<> h) noexcept {
    handler_ = h;
    int ret = co_conn_->StartSend();
    if (ret < 0) {
        result_.status = SEND_ERROR;
        result_.sent_len = -1;
        LogErrorf(logger_, "StartSend error:%d", ret);
        handler_.resume();
    }
}

SendResult CoTcpSendPromise::await_resume() const noexcept {
    return result_;
}

void CoTcpSendPromise::OnAwaiterSend(int sent_len) {
    if (sent_len > 0) {
        result_.status = SEND_OK;
        result_.sent_len = sent_len;
    } else {
        result_.status = SEND_ERROR;
        result_.sent_len = -1;
        LogErrorf(logger_, "uv_write error:%d", sent_len);
    }
    handler_.resume();
}

}