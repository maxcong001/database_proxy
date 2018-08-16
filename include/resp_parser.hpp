#pragma once
#include <utility>
#include <map>
#include <functional>
#include "logger/logger.hpp"
enum ParseResult
{
    Completed,
    Incompleted,
    Error,
};
class rasp_parser
{
  public:
    static std::pair<size_t, ParseResult> process_resp(std::string buf, std::function<void(char *, size_t)> on_resp_cb, size_t index = 0);

    static std::pair<size_t, ParseResult> process_array(std::string buf, size_t index);

    static std::pair<size_t, ParseResult> process_bulk(std::string buf, size_t index);

    static std::pair<size_t, ParseResult> process_integer(std::string buf, size_t index);

    static std::pair<size_t, ParseResult> process_error(std::string buf, size_t index);

    static std::pair<size_t, ParseResult> process_string(std::string buf, size_t index);
};