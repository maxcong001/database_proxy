
#include "logger.hpp"
#include "util.hpp"
#include "loop_thread_pool.hpp"
int main()
{
    set_log_level(logger_iface::log_level::debug);
    TASK_MSG msg;
    msg.type = TASK_MSG_TYPE::NEW_LISTENER;

    CONN_INFO conn;
    conn.type = CONN_TYPE::IP_V4;
    conn.IP = "127.0.0.1";
    conn.port = 6123;

    msg.body = conn;
    auto loop_threads = loop_thread_pool::instance();
    loop_threads->init();
    loop_threads->get_loop()->send2loop_thread(msg);





    getchar();
}
