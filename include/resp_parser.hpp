#pragma once
#include <utility>
#include <map>
#include <functional>
#include <algorithm>
#include "logger/logger.hpp"
#include "util.hpp"
enum ParseResult
{
    Completed,
    Incompleted,
    Error,
};
class rasp_parser
{
  public:
    static std::pair<size_t, ParseResult> process_resp(char *buf, size_t len, std::function<void(char *, size_t)> on_resp_cb, size_t index = 0);

    static std::pair<size_t, ParseResult> process_array(char *buf, size_t len, size_t index);

    static std::pair<size_t, ParseResult> process_bulk(char *buf, size_t len, size_t index);

    static std::pair<size_t, ParseResult> process_integer(char *buf, size_t len, size_t index);

    static std::pair<size_t, ParseResult> process_error(char *buf, size_t len, size_t index);

    static std::pair<size_t, ParseResult> process_string(char *buf, size_t len, size_t index);
};