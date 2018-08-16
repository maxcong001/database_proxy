#include "resp_parser.hpp"
static const char stringReply = '+';
static const char errorReply = '-';
static const char integerReply = ':';
static const char bulkReply = '$';
static const char arrayReply = '*';

size_t find_CRLF_index(char *buf, size_t len, size_t index)
{
    char *p = std::find(buf, buf + len, '\r');
    if (*(p + 1) == '\n')
    {
        return (unsigned long)p - (unsigned long)buf;
    }
    else
    {
        return 0;
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_string(char *buf, size_t len, size_t index)
{
    __LOG(debug, "now process string");
    std::size_t found = find_CRLF_index(buf, len, index);
    if (found)
    {
        return std::make_pair(found - index + 2, Completed);
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_error(char *buf, size_t len, size_t index)
{
    __LOG(debug, "now process error");
    std::size_t found = find_CRLF_index(buf, len, index);
    if (found)
    {
        return std::make_pair(found - index + 2, Completed);
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_integer(char *buf, size_t len, size_t index)
{
    __LOG(debug, "now process integer");
    std::size_t found = find_CRLF_index(buf, len, index);
    if (found)
    {
        return std::make_pair(found - index + 2, Completed);
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}


std::pair<size_t, ParseResult> rasp_parser::process_bulk(char *buf, size_t len, size_t index)
{
    __LOG(debug, "now process bulk");
    std::size_t found = find_CRLF_index(buf, len, index);
    if (found)
    {
        int bulk_len = 0;
        try
        {
            //bulk_len = std::stoi(buf.substr(index + 1, found - index - 1));
            bulk_len = std::stoi(std::string(buf + index + 1, found - index - 1));
        }
        catch (const std::invalid_argument &ia)
        {
            return std::make_pair(0, Error);
        }
        catch (const std::out_of_range &oa)
        {
            return std::make_pair(0, Error);
        }
        catch (...)
        {
            return std::make_pair(0, Error);
        }
        __LOG(debug, "the bulk len is : " << bulk_len);
        if (bulk_len == -1)
        {
            return std::make_pair(found - index + 2, Completed);
        }
        else if (bulk_len >= 0)
        {
            std::size_t found2 = find_CRLF_index(buf, len, found + 2);
            if (found2 != std::string::npos)
            {
                return std::make_pair(found2 - index + 2, Completed);
            }
            else
            {
                return std::make_pair(0, Incompleted);
            }
        }
        else
        {
            return std::make_pair(0, Error);
        }
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_array(char *buf, size_t len, size_t index)
{

    std::size_t found = find_CRLF_index(buf, len, index);
    size_t _sub_index = found + 2;
    if (found)
    {
        int array_size = 0;
        try
        {
            //array_size = std::stoi(buf.substr(index + 1, found - index - 1));
            array_size = std::stoi(std::string(buf + index + 1, found - index - 1));
            __LOG(debug, "now process array, array size is : " << array_size);
        }
        catch (const std::invalid_argument &ia)
        {
            return std::make_pair(0, Error);
        }
        catch (const std::out_of_range &oa)
        {
            return std::make_pair(0, Error);
        }
        catch (...)
        {
            return std::make_pair(0, Error);
        }

        if (array_size == 0 || array_size == -1)
        {
            return std::make_pair(found - index + 2, Completed);
        }
        else if (array_size < -1)
        {
            return std::make_pair(0, Error);
        }
        else
        {
            for (int i = 0; i < array_size; i++)
            {
                auto result = process_resp(buf, nullptr, _sub_index);
                if (std::get<1>(result) != Completed)
                {
                    return std::make_pair(0, Error);
                }
                else
                {
                    __LOG(debug, "now subindex is : " << _sub_index << ", will add : " << std::get<0>(result));
                    _sub_index += std::get<0>(result);
                }
            }
            __LOG(debug, "total index is : " << _sub_index);
            return std::make_pair((_sub_index - index), Completed);
        }
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}
std::pair<size_t, ParseResult> rasp_parser::process_resp(char *buf, size_t len, std::function<void(char *, size_t)> on_resp_cb, size_t index)
{
    size_t processed_buf = index;
    while (buf.size() > processed_buf)
    {
        __LOG(debug, "now process " << std::string((char *)&buf[processed_buf], 1));
        std::pair<size_t, ParseResult> result;
        switch (buf[processed_buf])
        {
        case stringReply:
        {
            result = process_string(buf, processed_buf);
        }
        break;
        case errorReply:
        {
            result = process_error(buf, processed_buf);
        }
        break;
        case integerReply:
        {
            result = process_integer(buf, processed_buf);
        }
        break;
        case bulkReply:
        {
            result = process_bulk(buf, processed_buf);
        }
        break;
        case arrayReply:
        {
            result = process_array(buf, processed_buf);
        }
        break;
        default:
            __LOG(error, "unknown char, not in : + - : $ *, char is : " << std::string((char *)&buf[processed_buf], 1));
            return std::make_pair(processed_buf - index, Error);
        }
        if (std::get<1>(result) != Completed)
        {
            return std::make_pair(processed_buf - index, std::get<1>(result));
        }
        else
        {
            if (on_resp_cb)
            {
                on_resp_cb(((char *)buf.data() + processed_buf), std::get<0>(result));
            }
            processed_buf += std::get<0>(result);
        }
    }
    return std::make_pair(processed_buf - index, Completed);
}
