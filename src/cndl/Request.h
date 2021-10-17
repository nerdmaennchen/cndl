#pragma once

#include <simplyfile/socket/Socket.h>

#include <memory>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace cndl {

struct Request {
    struct Header {
        using FieldMap = std::unordered_multimap<std::string, std::string>;

        struct BodyArg {
            FieldMap fields;
            std::vector<std::byte> content;
        };
        using BodyArgMap = std::unordered_multimap<std::string, BodyArg>;

        std::string method;
        std::string resource;    // the path for the resource (excl. parameters)
        std::string url;         // the requested URL (incl. parameters)
        std::string version;

        std::map<std::string, std::string> cookies;
        FieldMap fields;
        FieldMap url_args;

        BodyArgMap body_args;    // arguments that were passed via multipart/*

        std::size_t content_length{0};
    };

    using MessageBody = std::vector<std::byte>;
    Header header;
    MessageBody message_body;
};

using KV_Pair = std::pair<std::string, std::string>;
using FieldVal  = std::variant<std::string, KV_Pair>;

std::vector<FieldVal> extractFieldVals(std::string_view str, std::string_view delimiter=";,");

std::string url_unescape(std::string_view str);
Request::Header::FieldMap parse_fields(std::string_view fields);
Request::Header::BodyArgMap readMultipartBody(std::string_view body, std::string_view boundary);
Request::Header parse_header(std::string_view request);

}
