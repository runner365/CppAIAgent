#include "co_tcp_server.hpp"
#include "co_tcp_session_recv.hpp"
#include "co_tcp_session_send.hpp"
#include "co_tcp_accept_conn.hpp"
#include "utils/logger.hpp"
#include "utils/co_pub.hpp"

#include <iostream>
#include <uv.h>
#include <coroutine>

using namespace cpp_streamer;

static int listion_port = 0;

CoVoidTask EchoRecv(std::shared_ptr<TcpCoAcceptConn> accept_conn, Logger* logger) {
    const size_t buffer_size = 10 * 1024;
    uint8_t recv_data[buffer_size] = {0};

    while(true) {
        RecvResult recv_result = co_await accept_conn->CoReceive(recv_data, buffer_size);
        if (recv_result.status != RECV_OK) {
            LogErrorf(logger, "Failed to receive data, status: %d", recv_result.status);
            co_return;
        }
        std::string received_message(reinterpret_cast<char*>(recv_data), recv_result.recv_len);
        LogInfof(logger, "Received data: %s", received_message.c_str());
        
        SendResult send_result = co_await accept_conn->CoSend(recv_data, recv_result.recv_len);
        if (send_result.status != SEND_OK) {
            LogErrorf(logger, "Failed to send data, status: %d", send_result.status);
            co_return;
        }
        LogInfof(logger, "Sent data length: %d, status: %d\r\n", send_result.sent_len, send_result.status);
    }
    co_return;
}

CoVoidTask Run(CoTcpServer* server) {
    Logger* logger = server->logger_;
    LogInfof(logger, "Starting Echo Server on port %d", listion_port);
    std::shared_ptr<TcpCoAcceptConn> accept_conn;

    while (true) {
        accept_conn = co_await server->CoAccept();
        if (accept_conn->handle_ == nullptr || accept_conn->status_ != TCP_CONNECT_SUCCESS) {
            LogErrorf(logger, "Failed to accept connection, status: %d", accept_conn->status_);
            break;
        }
        LogInfof(logger, "Accepted new connection, handle: %p, status: %d", accept_conn->handle_, accept_conn->status_);
        EchoRecv(accept_conn, logger);
    }
    co_return;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    listion_port = atoi(argv[1]);
    if (listion_port <= 0 || listion_port > 65535) {
        std::cerr << "Invalid port number: " << listion_port << std::endl;
        return 1;
    }

    uv_loop_t* loop = uv_default_loop();
    Logger* logger = new Logger("server.log");
    logger->EnableConsole();

    CoTcpServer server(loop, "0.0.0.0", listion_port, logger);

    Run(&server);
    LogInfof(logger, "uv loop is runnning");
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}