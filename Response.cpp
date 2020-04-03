#include "Response.h"

#include <algorithm>
#include <map>
#include <string_view>

namespace cndl {

namespace {

const std::unordered_map<int, std::string_view> code2reason {
    // 1xx switching protocols
    {101, "Switching Protocols"},

    // 2xx success
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},

    // 3xx redirection
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "Switch Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},

    // 4xx client errors
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},

    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},

    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},

    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},


    // 5xx server errors
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"},
};

}

Response::Response(Error const& from_error, ErrorBodyGenerator pageGenerator)
  : Response{}
{
    status_code = from_error.code();

    if (pageGenerator) {
        std::string body = pageGenerator(status_code, from_error.what());
        std::transform(begin(body), end(body), std::back_inserter(message_body), [](char c) { return std::byte{static_cast<unsigned char>(c)}; });
    } else {
        std::string_view what{from_error.what()};
        std::transform(begin(what), end(what), std::back_inserter(message_body), [](char c) { return std::byte{static_cast<unsigned char>(c)}; });
    }
}

std::vector<std::byte> Response::serialize() const {
    using namespace std::string_view_literals;

    std::vector<std::byte> serialized;
    auto append_str = [](std::vector<std::byte>& target, auto const& str) {
        std::transform(begin(str), end(str), std::back_inserter(target), [](char c) { return std::byte{static_cast<unsigned char>(c)}; });
    };

    append_str(serialized, version);
    append_str(serialized, " "sv);
    append_str(serialized, std::to_string(status_code));
    append_str(serialized, " "sv);

    if (reason_phrase.empty()) {
        append_str(serialized, code2reason.at(status_code));
    } else {
        append_str(serialized, reason_phrase);
    }
    append_str(serialized, "\r\n"sv);

    for (auto const& [name, val] : fields) {
        append_str(serialized, name);
        append_str(serialized, ": "sv);
        append_str(serialized, val);
        append_str(serialized, "\r\n"sv);
    }


    append_str(serialized, "Content-Length: "sv);
    append_str(serialized, std::to_string(message_body.size()));
    append_str(serialized, "\r\n\r\n"sv);

    serialized.insert(serialized.end(), message_body.begin(), message_body.end());

    return serialized;
}

}
