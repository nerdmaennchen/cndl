#pragma once

#include "error.h"

#include <map>
#include <vector>
#include <cstddef>

#include <functional>

namespace cndl {

struct Response {
    using ErrorBodyGenerator = std::function<std::string(int code, std::string_view msg)>;
    using AfterSentHandler   = std::function<bool(Response& response)>;

    using MessageBody = std::vector<std::byte>;

    std::string version {"HTTP/1.1"};
    int status_code {200};
    std::string reason_phrase {""}; // if empty, will be auto filled
    std::map<std::string, std::string> fields{{"Content-Type", "text/html, charset=utf-8"}};

    MessageBody message_body;

    Response() = default;
    Response(Error const& from_error, ErrorBodyGenerator pageGenerator={});

    template<typename CharT>
    Response(std::basic_string_view<CharT> body) : Response{} {
        std::transform(begin(body), end(body), std::back_inserter(message_body), [](char c) { return std::byte{static_cast<unsigned char>(c)}; });
    }

    template<typename CharT>
    Response(std::basic_string<CharT> const& body) : Response{} {
        std::transform(begin(body), end(body), std::back_inserter(message_body), [](char c) { return std::byte{static_cast<unsigned char>(c)}; });
    }

    std::vector<std::byte> serialize() const;
};

}