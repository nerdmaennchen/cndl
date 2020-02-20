#pragma once

#include "simplyfile/socket/Socket.h"

#include <string_view>
#include <unordered_map>
#include <memory>
#include <variant>

namespace cndl {


struct Request_Header {
    using FieldMap = std::unordered_multimap<std::string, std::string>;

    struct BodyArg {
        FieldMap fields;
        std::vector<std::byte> content;
    };
    using BodyArgMap = std::unordered_multimap<std::string, BodyArg>;

    std::string method;
    std::string ressource;    // the path for the ressource (excl. parameters)
    std::string url;          // the requested URL (incl. parameters)
    std::string version;

    FieldMap fields;
    FieldMap url_args;

    BodyArgMap body_args;     // arguments that were passed via multipart/*

    std::size_t content_length{0};
};

struct Request {
    using MessageBody = std::vector<std::byte>;
    Request_Header header;
    MessageBody message_body;
};

using KV_Pair = std::pair<std::string, std::string>;
using FieldVal  = std::variant<std::string, KV_Pair>;

std::vector<FieldVal> extractFieldVals(std::string_view str, std::string_view delimiter=";,");

std::string url_unescape(std::string_view str);
Request_Header::FieldMap parse_fields(std::string_view fields);
Request_Header::BodyArgMap readMultipartBody(std::string_view body, std::string_view boundary);
Request_Header parse_header(std::string_view request);
}