
#include "logger/logger.hpp"

std::int32_t main()
{
    set_log_level(logger_iface::log_level::warn);
    set_max_log_buff(10);
    for (std::int32_t i = 0; i < 100; i++)
    {
        __LOG(error, "hello logger!"
                         << "this is error log");
        __LOG(warn, "hello logger!"
                        << "this is warn log");
        __LOG(info, "hello logger!"
                        << "this is info log");
        __LOG(debug, "hello logger!"
                         << "this is debug log");
    }
    dump_log();
}
