#pragma once

#include "Error.h"
#include "unique_function.h"

#include <map>
#include <vector>
#include <cstddef>
#include <span>
#include <functional>
#include <variant>
#include <chrono>
#include <optional>

#include <time.h>

namespace cndl {

struct CookieAttributes {
    std::variant<std::monostate, struct tm, std::chrono::seconds> lifetime{std::monostate{}};
    bool http_only {false};
    bool secure {true};
    std::optional<std::string> path;
    std::optional<std::string> domain;
};

struct Response {
    using ErrorBodyGenerator = std::function<std::string(int code, std::string_view msg)>;
    using AfterSentHandler   = unique_func<bool(Response& response)>;

    using MessageBody = std::vector<std::byte>;

    std::string version {"HTTP/1.1"};
    int status_code {200};
    std::string reason_phrase {""}; // if empty, will be auto filled
    std::multimap<std::string, std::string> fields{{"Content-Type", "text/html, charset=utf-8"}};

    MessageBody message_body;

    Response() = default;
    Response(Error const& from_error, ErrorBodyGenerator pageGenerator={});

    template<typename CharT>
    Response(std::span<const CharT> body) {
        auto bytes = std::as_bytes(body);
        message_body = MessageBody{bytes.begin(), bytes.end()};
    }

    Response(std::string_view body) : Response{std::span{body.data(), body.size()}} {
    }

    Response(std::string const& body) : Response{std::span{body.data(), body.size()}} {
    }
    
    Response(std::vector<std::byte> body) : message_body{std::move(body)} {
    }

    void setCookie(std::string_view name, std::string_view value, CookieAttributes attribues={});

    void setContentTypeFromExtension(std::string_view extension);
    
    static std::string_view contentTypeLookup(std::string_view extension);

    std::vector<std::byte> serialize() const;
};

struct AsyncResponse {
    struct ChunkSender {
        using FlushFtor = unique_func<void(std::span<const std::byte>)>;
        ChunkSender(FlushFtor flushFtor) : flush{std::move(flushFtor)} {
        }

        template<typename CharT>
        void operator()(std::span<const CharT> data) {
            flush(data.as_bytes());
        }

    private:
        FlushFtor flush;
    };
    using HookFtor = unique_func<void(ChunkSender)>;
    HookFtor hook;
};

}
