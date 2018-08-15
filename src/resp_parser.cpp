#include "resp_parser.hpp"
static const char stringReply = '+';
static const char errorReply = '-';
static const char integerReply = ':';
static const char bulkReply = '$';
static const char arrayReply = '*';

std::pair<size_t, ParseResult> rasp_parser::process_string(std::string buf, size_t index)
{
    std::size_t found = buf.find("\r\n", index);
    if (found != std::string::npos)
    {
        return std::make_pair(found - index + 2, Completed);
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_error(std::string buf, size_t index)
{
    std::size_t found = buf.find("\r\n", index);
    if (found != std::string::npos)
    {
        return std::make_pair(found - index + 2, Completed);
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_integer(std::string buf, size_t index)
{
    std::size_t found = buf.find("\r\n", index);
    if (found != std::string::npos)
    {
        return std::make_pair(found - index + 2, Completed);
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}

std::pair<size_t, ParseResult> rasp_parser::process_bulk(std::string buf, size_t index)
{
    std::size_t found = buf.find("\r\n", index);
    if (found != std::string::npos)
    {
        int bulk_len = 0;
        try
        {
            std::stoi(buf.substr(index + 1, found - index - 1));
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

        if (bulk_len == -1)
        {
            return std::make_pair(found - index + 2, Completed);
        }
        else if (bulk_len >= 0)
        {
            std::size_t found2 = buf.find("\r\n", found + 2);
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

std::pair<size_t, ParseResult> rasp_parser::process_array(std::string buf, size_t index)
{
    std::size_t found = buf.find("\r\n", index);
    if (found != std::string::npos)
    {
        int array_size = 0;
        try
        {
            std::stoi(buf.substr(index + 1, found - index - 1));
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
            size_t _sub_index = found + 2;
            for (int i = 0; i < array_size; i++)
            {
                auto result = process_resp(buf, nullptr, _sub_index);
                if (std::get<1>(result) != Completed)
                {
                    return std::make_pair(0, Error);
                }
                else
                {
                    _sub_index += std::get<0>(result);
                }
            }
        }
    }
    else
    {
        return std::make_pair(0, Incompleted);
    }
}
std::pair<size_t, ParseResult> rasp_parser::process_resp(std::string buf, std::function<void(char *, size_t)> on_resp_cb, size_t index)
{
    size_t processed_buf = index;
    while (buf.size() > processed_buf)
    {
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
            return std::make_pair(processed_buf, Error);
        }
        if (std::get<1>(result) != Completed)
        {
            return std::make_pair(processed_buf, std::get<1>(result));
        }
        else
        {
            on_resp_cb(((char *)buf.data() + processed_buf), std::get<0>(result));
            processed_buf += std::get<0>(result);
        }
    }
}
