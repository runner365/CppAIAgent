#include "co_tcp_conn.hpp"
#include "co_tcp_conn_send.hpp"
#include "co_tcp_conn_recv.hpp"
#include "utils/logger.hpp"
#include "utils/co_pub.hpp"
#include "utils/timer.hpp"
#include <coroutine>
#include <uv.h>
#include <memory>

using namespace cpp_streamer;

const std::string send_msg = "Isaac Newton (1643–1727) was an English physicist, mathematician, and astronomer, \
widely regarded as one of the most influential scientists in history. ";

class EchoClient : public TimerInterface
{
public:
    EchoClient(uv_loop_t* loop, const std::string& host, int port, Logger* logger, bool ssl_enable)
        : TimerInterface(loop, 4000), loop_(loop), host_(host), port_(port), logger_(logger), ssl_enable_(ssl_enable) {
        StartTimer();
    }

    virtual ~EchoClient() {
        StopTimer();
    }

protected:
    CoVoidTask DoJob() {
        std::shared_ptr<TcpCoConn> conn_ptr = std::make_shared<TcpCoConn>(loop_, host_, port_, ssl_enable_, logger_);

        LogInfof(logger_, "Echo Connecting to %s:%d", host_.c_str(), port_);
        TCP_CONNECT_STATUS conn_status = co_await conn_ptr->CoConnect();
        if (conn_status == TCP_CONNECT_SUCCESS) {
            LogInfof(logger_, "Echo Connected to %s:%d", host_.c_str(), port_);
        } else if (conn_status == TCP_CONNECT_FAILED) {
            LogErrorf(logger_, "Echo Failed to connect to %s:%d", host_.c_str(), port_);
            is_job_running_ = false;
            co_return;
        } else {
            LogInfof(logger_, "Echo Connection pending to %s:%d", host_.c_str(), port_);
            is_job_running_ = false;
            co_return;
        }

        do {
            SendResult send_result = co_await conn_ptr->CoSend(reinterpret_cast<const uint8_t*>(send_msg.c_str()), send_msg.length());
            if (send_result.status == SEND_OK) {
                LogInfof(logger_, "Echo Sent data, length: %d", send_result.sent_len);
            } else {
                LogErrorf(logger_, "Echo Send failed, status: %d", send_result.status);
                is_job_running_ = false;
                co_return;
            }
            uint8_t recv_data[10*1024] = {0};

            LogInfof(logger_, "Echo reading for data...");
            RecvResult recv_result = co_await conn_ptr->CoReceive(recv_data, sizeof(recv_data));
            if (recv_result.status == RECV_ERROR) {
                LogErrorf(logger_, "Echo Receive failed, status: %d", recv_result.status);
                is_job_running_ = false;
                co_return;
            } else if (recv_result.status == RECV_PENDING) {
                LogInfof(logger_, "Echo Receive pending");
                is_job_running_ = false;
                co_return;
            }
            std::string recv_str(reinterpret_cast<const char*>(recv_data), recv_result.recv_len);
            LogInfof(logger_, "Echo Received data: %s", recv_str.c_str());
        } while (true);
    }
protected:
    virtual void OnTimer() override {
        if (!is_job_running_) {
            is_job_running_ = true;
            DoJob();
        }
    }
private:
    uv_loop_t* loop_ = nullptr;
    std::string host_;
    int port_ = 0;
    Logger* logger_ = nullptr;
    bool ssl_enable_ = false;

private:
    bool is_job_running_ = false;
};

int main(int argn, char** argv) {
    // Example usage of CoTcpConnectPromise
    uv_loop_t* loop = uv_default_loop();
    
    Logger* logger = new Logger();
    logger->SetFilename("echo_client_co.log");
    logger->SetLevel(LOGGER_INFO_LEVEL);
    logger->EnableConsole();

    EchoClient client(loop, "127.0.0.1", 8080, logger, false);

    LogInfof(logger, "Starting event loop...");
    while (true) {
        uv_run(loop, UV_RUN_DEFAULT);
    }
    delete logger;

    return 0;
}