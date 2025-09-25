#include "co_tcp_session_send.hpp"
#include "co_tcp_accept_conn.hpp"

namespace cpp_streamer
{
CoTcpSessionSendPromise::CoTcpSessionSendPromise(TcpCoAcceptConn* accept_conn, const uint8_t* data, size_t size, Logger* logger)
    : accept_conn_(accept_conn), logger_(logger)
{
    send_data_ = new uint8_t[size];
    send_size_ = size;
    memcpy(send_data_, data, size);
}

CoTcpSessionSendPromise::~CoTcpSessionSendPromise()
{
    if (send_data_) {
        delete[] send_data_;
    }
    send_data_ = nullptr;
    send_size_ = 0;
}

void CoTcpSessionSendPromise::HandleSend(int status, size_t sent_len) {
    if (status == 0) {
        result_.status = SEND_OK;
        result_.sent_len = (int)sent_len;
    } else {
        LogErrorf(logger_, "CoTcpSessionSendPromise OnWrite error, status: %d", status);
        result_.status = SEND_ERROR;
        result_.sent_len = -1;
    }
    handler_.resume();
}

bool CoTcpSessionSendPromise::await_ready() const noexcept {
    return false;
}

void CoTcpSessionSendPromise::await_suspend(std::coroutine_handle<> h) noexcept {
    handler_ = h;
    int ret = accept_conn_->SendData(send_data_, send_size_);
    if (ret < 0) {
        LogErrorf(logger_, "CoTcpSessionSendPromise await_suspend SendData error, ret: %d", ret);
        result_.status = SEND_ERROR;
        result_.sent_len = -1;
        handler_.resume();
        return;
    }
    accept_conn_->AddSendAwaiter(this);
}

SendResult CoTcpSessionSendPromise::await_resume() const noexcept {
    return result_;
}
}