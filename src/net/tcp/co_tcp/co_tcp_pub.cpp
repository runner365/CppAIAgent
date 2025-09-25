#include "co_tcp_pub.hpp"
#include "utils/ipaddress.hpp"
#include <sstream>

namespace cpp_streamer
{
void OnCoConnUVClose(uv_handle_t *handle) {
    if (handle) {
        free(handle);
    }
}
}
