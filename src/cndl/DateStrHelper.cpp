#include "DateStrHelper.h"

#include <string_view>
#include <array>

#include <cstdio>

using namespace std::string_view_literals;

namespace cndl {

namespace {
constexpr auto date_format_string = "%a, %d %b %Y %H:%M:%S %Z"sv;
}

std::string mkdatestr(struct tm const& tm) {
    std::array<char, 128> date_buf;
    strftime(date_buf.data(), date_buf.size(), date_format_string.data(), &tm);
    return std::string(date_buf.data());
}

std::string mkdatestr(struct timespec const& ts) {
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);
    return mkdatestr(tm);
}

struct tm parsedatestr(std::string_view str) {
    struct tm req_tm{};
    strptime(str.data(), date_format_string.data(), &req_tm);
    return req_tm;
}

}