#include "co_tcp_server.hpp"
#include "co_tcp_accept_conn.hpp"
#include "co_tcp_pub.hpp"
#include "utils/logger.hpp"
#include <stdio.h>

namespace cpp_streamer
{
void on_uv_connection(uv_stream_t* handle, int status) {
    CoTcpServer* server = static_cast<CoTcpServer*>(handle->data);
    if (server) {
        server->OnConnection(status, handle);
    }
}

CoTcpServerPromise::CoTcpServerPromise(CoTcpServer* server,
                Logger* logger): logger_(logger), server_(server)
{
}

CoTcpServerPromise::~CoTcpServerPromise()
{
}

void CoTcpServerPromise::AcceptConnection(int status, uv_stream_t* handle) {
    current_status_ = ((status == 0) ? TCP_CONNECT_SUCCESS : TCP_CONNECT_FAILED);
    current_handle_ = handle;

    handle_.resume();
}

bool CoTcpServerPromise::await_ready() const noexcept {
    return false;
}

void CoTcpServerPromise::await_suspend(std::coroutine_handle<> h) noexcept {
    handle_ = h;

    server_->AddAwaiter(this);
}

std::shared_ptr<TcpCoAcceptConn> CoTcpServerPromise::await_resume() const noexcept {
    uv_tcp_t* uv_tcp_handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    uv_tcp_handle->data = nullptr;

    uv_tcp_init(server_->loop_, uv_tcp_handle);
    uv_accept(
      reinterpret_cast<uv_stream_t*>(current_handle_),
      reinterpret_cast<uv_stream_t*>(uv_tcp_handle));
    return std::make_shared<TcpCoAcceptConn>(current_handle_, uv_tcp_handle, current_status_, logger_);
}

CoTcpServer::CoTcpServer(uv_loop_t* loop,
                const std::string& host,
                int port,
                Logger* logger):loop_(loop)
                            , host_(host)
                            , port_(port)
                            , logger_(logger)
{
    server_handle_.data = this;
    uv_ip4_addr(host_.c_str(), port_, &server_addr_);
    uv_tcp_init(loop_, &server_handle_);
    uv_tcp_bind(&server_handle_, (const struct sockaddr*)&server_addr_, 0);
    uv_listen((uv_stream_t*)&server_handle_, SOMAXCONN, on_uv_connection);
}

CoTcpServer::~CoTcpServer()
{
}

CoTcpServerPromise CoTcpServer::CoAccept() {
    return CoTcpServerPromise(this, logger_);

}

void CoTcpServer::OnConnection(int status, uv_stream_t* handle) {
    if (awaiters_.empty()) {
        LogInfof(logger_, "No awaiters available, connection will be ignored");
        return;
    }
    auto awaiter_it = awaiters_.front();
    awaiters_.pop();
    awaiter_it->AcceptConnection(status, handle);
}

void CoTcpServer::AddAwaiter(CoTcpServerPromise* awaiter) {
    awaiters_.push(awaiter);
}
}