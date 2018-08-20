
#include "logger/logger.hpp"

std::int32_t main()
{
    set_log_level(logger_iface::log_level::warn);
    TASK_MSG msg;
    msg.type = TASK_MSG_TYPE::NEW_LISTENER;

    CONN_INFO conn;
    conn.type = IP_v4;
    conn.IP = "127.0.0.1" conn.port = 6379;

    msg.body = conn;
    loop_thread_pool::instance()->get_loop()->send2loop_thread(msg);

    getchar();
}
