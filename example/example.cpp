
#include "logger.hpp"
#include "util.hpp"
#include "loop_thread_pool.hpp"
int main()
{
    set_log_level(logger_iface::log_level::debug);
    __LOG(warn, "now start a listener");
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
    __LOG(warn, "now add a connection to redis");
    msg.type = TASK_MSG_TYPE::ADD_CONN;
    conn.type = CONN_TYPE::IP_V4;
    conn.IP = "127.0.0.1";
    conn.port = 6379;

    msg.body = conn;
    loop_threads->get_loop()->send2loop_thread(msg);

    getchar();
    __LOG(warn, "now start a new connection to listener");
    conn.type = CONN_TYPE::IP_V4;
    conn.IP = "127.0.0.1";
    conn.port = 6123;
    TcpClient _test_client(loop_threads->get_loop()->get_loop());
    __LOG(debug, "now connect to listener");
    _test_client.connect(conn);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    getchar();
    __LOG(warn, "now send to listener");
    // just for test. should not send in this thread
    std::string test_str("+PING\r\n");
    _test_client.send(test_str.c_str(), test_str.size());
    getchar();
}
