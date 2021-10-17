#include "Response.h"
#include "DateStrHelper.h"

#include <algorithm>
#include <map>
#include <string_view>

namespace cndl {

using namespace std::string_literals;
using namespace std::string_view_literals;

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

const std::unordered_map<std::string_view, std::string_view> extension_lookup_table {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"shtml", "text/html"},
    {"css", "text/css"},
    {"xml", "text/xml"},
    {"gif", "image/gif"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"js", "application/javascript"},
    {"atom", "application/atom+xml"},
    {"rss", "application/rss+xml"},
    {"mml", "text/mathml"},
    {"txt", "text/plain"},
    {"jad", "text/vnd.sun.jme.app-descriptor"},
    {"wml", "text/vnd.wap.wml"},
    {"htc", "text/x-component"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"svgz", "image/svg+xml"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"wbmp", "image/vnd.wap.wbmp"},
    {"webp", "image/webp"},
    {"ico", "image/x-icon"},
    {"jng", "image/x-jng"},
    {"bmp", "image/x-ms-bmp"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"jar", "application/java-archive"},
    {"war", "application/java-archive"},
    {"ear", "application/java-archive"},
    {"json", "application/json"},
    {"hqx", "application/mac-binhex40"},
    {"doc", "application/msword"},
    {"pdf", "application/pdf"},
    {"ps", "application/postscript"},
    {"eps", "application/postscript"},
    {"ai", "application/postscript"},
    {"rtf", "application/rtf"},
    {"m3u8", "application/vnd.apple.mpegurl"},
    {"kml", "application/vnd.google-earth.kml+xml"},
    {"kmz", "application/vnd.google-earth.kmz"},
    {"xls", "application/vnd.ms-excel"},
    {"eot", "application/vnd.ms-fontobject"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"odg", "application/vnd.oasis.opendocument.graphics"},
    {"odp", "application/vnd.oasis.opendocument.presentation"},
    {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {"odt", "application/vnd.oasis.opendocument.text"},
    {"wmlc", "application/vnd.wap.wmlc"},
    {"7z", "application/x-7z-compressed"},
    {"cco", "application/x-cocoa"},
    {"jardiff", "application/x-java-archive-diff"},
    {"jnlp", "application/x-java-jnlp-file"},
    {"run", "application/x-makeself"},
    {"pl", "application/x-perl"},
    {"pm", "application/x-perl"},
    {"prc", "application/x-pilot"},
    {"pdb", "application/x-pilot"},
    {"rar", "application/x-rar-compressed"},
    {"rpm", "application/x-redhat-package-manager"},
    {"sea", "application/x-sea"},
    {"swf", "application/x-shockwave-flash"},
    {"sit", "application/x-stuffit"},
    {"tcl", "application/x-tcl"},
    {"tk", "application/x-tcl"},
    {"der", "application/x-x509-ca-cert"},
    {"pem", "application/x-x509-ca-cert"},
    {"crt", "application/x-x509-ca-cert"},
    {"xpi", "application/x-xpinstall"},
    {"xhtml", "application/xhtml+xml"},
    {"xspf", "application/xspf+xml"},
    {"zip", "application/zip"},
    {"bin", "application/octet-stream"},
    {"exe", "application/octet-stream"},
    {"dll", "application/octet-stream"},
    {"deb", "application/octet-stream"},
    {"dmg", "application/octet-stream"},
    {"iso", "application/octet-stream"},
    {"img", "application/octet-stream"},
    {"msi", "application/octet-stream"},
    {"msp", "application/octet-stream"},
    {"msm", "application/octet-stream"},
    {"mid", "audio/midi"},
    {"midi", "audio/midi"},
    {"kar", "audio/midi"},
    {"mp3", "audio/mpeg"},
    {"ogg", "audio/ogg"},
    {"m4a", "audio/x-m4a"},
    {"ra", "audio/x-realaudio"},
    {"3gpp", "video/3gpp"},
    {"3gp", "video/3gpp"},
    {"ts", "video/mp2t"},
    {"mp4", "video/mp4"},
    {"mpeg", "video/mpeg"},
    {"mpg", "video/mpeg"},
    {"mov", "video/quicktime"},
    {"webm", "video/webm"},
    {"flv", "video/x-flv"},
    {"m4v", "video/x-m4v"},
    {"mng", "video/x-mng"},
    {"asx", "video/x-ms-asf"},
    {"asf", "video/x-ms-asf"},
    {"wmv", "video/x-ms-wmv"},
    {"avi", "video/x-msvideo"},
    {"wasm", "application/wasm"},
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

void Response::setCookie(std::string_view name, std::string_view value, CookieAttributes attributes) {
    std::string val = std::string{name} + "=" + std::string{value};
    if (auto expires = std::get_if<struct tm>(&attributes.lifetime); expires) {
        val += "; Expires=" + mkdatestr(*expires);
    }
    if (auto max_age = std::get_if<std::chrono::seconds>(&attributes.lifetime); max_age) {
        val += "; Max-Age=" + std::to_string(max_age->count()) + "s";
    }
    if (attributes.secure) {
        val += "; Secure";
    }
    if (attributes.http_only) {
        val += "; HttpOnly";
    }
    if (attributes.path) {
        val += "; Path="+*attributes.path;
    }
    if (attributes.domain) {
        val += "; Domain="+*attributes.domain;
    }

    fields.emplace("Set-Cookie", std::move(val));
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


void Response::setContentTypeFromExtension(std::string_view extension) {
    fields.erase("Content-Type");
    fields.emplace("Content-Type", contentTypeLookup(extension));
}

std::string_view Response::contentTypeLookup(std::string_view extension) {
    if (extension.size() > 0 and extension[0] == '.') {
        extension = extension.substr(1);
    }
    auto it = extension_lookup_table.find(extension);
    if (it == extension_lookup_table.end()){
        return "text/plain";
    }
    return it->second;
}

}
