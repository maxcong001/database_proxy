
#include "logger.hpp"
#include "util.hpp"
#include "loop_thread_pool.hpp"
#include <vector>

int main()
{
    set_log_level(logger_iface::log_level::error);
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
    /*
    loop_threads->get_loop_thread()->send2loop_thread(msg);
    getchar();
    __LOG(warn, "now add a connection to redis");
    msg.type = TASK_MSG_TYPE::ADD_CONN;
    conn.type = CONN_TYPE::IP_V4;
    conn.IP = "127.0.0.1";
    conn.port = 6379;

    msg.body = conn;
    */
    loop_threads->send_to_all(msg);

    getchar();
    {
        std::vector<std::shared_ptr<TcpClient>> _tcp_conns;
        for (int i = 1; i < 100; i++)
        {
            __LOG(warn, "now start a new connection to listener");
            conn.type = CONN_TYPE::IP_V4;
            conn.IP = "127.0.0.1";
            conn.port = 6123;
            std::shared_ptr<TcpClient> _test_client_sptr = std::make_shared<TcpClient>(loop_threads->get_loop_thread()->get_loop());
            __LOG(debug, "now connect to listener");
            _test_client_sptr->connect(conn);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            _tcp_conns.push_back(_test_client_sptr);
        }
        __LOG(warn, "new connection done");
        getchar();
        std::string test_str("*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$8\r\nmy value\r\n");
        __LOG(warn, "now send to listener with size :" << test_str.size());
        // just for test. should not send in this thread

        for (auto it : _tcp_conns)
        {
            it->send(test_str.c_str(), test_str.size());
        }

        __LOG(warn, "exit client");
        getchar();
    }
    getchar();
}
