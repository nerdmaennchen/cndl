#include "HttpProtocol.h"
#include "ConnectionHandler.h"
#include "Dispatcher.h"
#include "overloaded.h"
#include "base64.h"

#include <simplyfile/socket/Socket.h>
#include <simplyfile/Epoll.h>

#include <algorithm>
#include <iostream>

#include <openssl/sha.h>

namespace cndl {

using namespace std::string_view_literals;

namespace {

constexpr auto magic_ws_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"sv;

bool starts_with(std::string_view l, std::string_view prefix) {
    return l.size() >= prefix.size() and l.substr(0, prefix.size()) == prefix;
};

bool ignore_case_cmp(std::string_view l, std::string_view r) {
    if (l.size() != r.size()) {
        return false;
    }

    return std::equal(begin(l), end(l), begin(r), end(r), [](char l, char r) {
        return ::tolower(l) == ::tolower(r);
    });

}

ProtocolHandler::ProtocolChange connection_upgrade(Request const& request, ConnectionHandler& handler) {
    auto const& header = request.header;
    if (header.method != "GET") {
        throw Error{405};
    }

    auto const& fields = header.fields;
    auto upgrade_type = fields.find("upgrade");
    if (upgrade_type == fields.end()) {
        throw Error{400, "bad upgrade"};
    }
    if (not ignore_case_cmp(upgrade_type->second, "websocket"sv)) {
        throw Error{501, "unknown upgrade"};
    }

    auto websocket_key      = fields.find("sec-websocket-key");
    auto websocket_version  = fields.find("sec-websocket-version");

    if (websocket_key == fields.end() or
        websocket_version == fields.end()) {
        throw Error{400, "bad upgrade"};
    }
    if (websocket_version->second != "13") {
        throw Error{501, "bad websocket version"};
    }


    std::array<std::byte, 20> hash;
    std::string combined = websocket_key->second + std::string{magic_ws_guid};
    SHA1(reinterpret_cast<std::uint8_t const*>(combined.data()), combined.size(), reinterpret_cast<std::uint8_t*>(hash.data()));

    Response response;
    response.status_code = 101;
    response.fields.emplace("Upgrade", "websocket");
    response.fields.emplace("Connection", "Upgrade");
    response.fields.emplace("Sec-WebSocket-Accept", cndl::base64_encode({hash.data(), hash.size()}));

    auto ws = std::make_unique<Websocket>(&handler);

    auto& route = handler.getDispatcher().routeWS(request);
    handler.write(response.serialize());
    route.onOpen(request, *ws);
    ws->setHandler(route.getHandler());

    
    return ProtocolHandler::ProtocolChange{std::move(ws)};
}

}

HttpProtocol::ConsumeResult HttpProtocol::onDataReceived(ByteView received) {
    int consumed = 0;
    auto& dispatcher = connection_handler->getDispatcher();
    ProtocolHandler::ProtocolChange protocol_change;

    // std::cout << std::string_view(reinterpret_cast<char const*>(received.data()), received.size()) << std::endl;
    while (not protocol_change) {
        try {
            if (not header) {
                // test if there is a header in the buffer
                ByteView crlfcrlf{reinterpret_cast<std::byte const*>("\r\n\r\n"), 4};
                auto header_end_idx = received.find(crlfcrlf);
                if (header_end_idx != ByteView::npos) {
                    header = parse_header(std::string_view{reinterpret_cast<char const*>(received.begin()), header_end_idx+2});
                    consumed += header_end_idx+4;
                    received = received.substr(consumed);
                }
            }

            if (header) {
                if (received.size() < header->content_length) {
                    return {consumed, ProtocolHandler::ProtocolChange{}}; // content missing
                }

                Request::MessageBody message{received.begin(), received.begin() + header->content_length};
                received = received.substr(header->content_length);
                consumed += header->content_length;

                Request::Header processed_header{std::move(*header)};
                header.reset();

                auto enctype = processed_header.fields.find("content-type");
                if (enctype != processed_header.fields.end()) {
                    auto split = extractFieldVals(enctype->second);
                    if (split.empty()) {
                        throw Error(400, "content-type must be defined");
                    }
                    if (std::holds_alternative<std::string>(split[0]) and starts_with(std::get<std::string>(split[0]),  "multipart/"sv)) {
                        if (split.size() < 2) {
                            throw Error(400, "content-type: multipart requires a boundary");
                        }

                        auto bd_it = std::find_if(begin(split), end(split), [](auto const& s){
                            return std::visit(detail::overloaded{
                                [](auto const&) { return false; },
                                [](KV_Pair const& pair) { return pair.first=="boundary"; },
                            }, s);
                        });

                        if (bd_it == std::end(split)) {
                            throw Error(400, "content-type: multipart requires a boundary");
                        }

                        auto boundary = std::get<KV_Pair>(*bd_it).second;
                        processed_header.body_args = readMultipartBody(
                            std::string_view{reinterpret_cast<char*>(message.data()),
                            message.size()}, boundary
                        );
                    }
                }

                auto con_field = processed_header.fields.find("connection");
                bool con_upgrade = false;
                if (con_field != processed_header.fields.end()) {
                    auto split = extractFieldVals(con_field->second, ",;");
                    con_upgrade = std::find_if(begin(split), end(split), [](auto const& v){
                        return std::holds_alternative<std::string>(v) and ignore_case_cmp(std::get<std::string>(v), "upgrade"sv);
                    }) != std::end(split);
                }
                Request request{std::move(processed_header), std::move(message)};
                // std::cout << request.header.method << " " << request.header.url << std::endl;
                if (con_upgrade) {
                    protocol_change = connection_upgrade(std::move(request), *connection_handler);
                    break;
                } else {
                    // dispatch request
                    auto response = dispatcher.route(std::move(request));
                    connection_handler->write(response.serialize());

                    if (processed_header.version=="HTTP/1.0") {
                        protocol_change = nullptr; // this means no further data will be passes to this connection_handler
                    }
                }
            } else {
                break;
            }
        } catch (Error const& err) {
            connection_handler->write(Response(err, dispatcher.getErrorBodyGenerator()).serialize());
            if (err.code() >= 500) {
                protocol_change = nullptr; // this means no further data will be passes to this connection_handler
            }
        } catch (std::runtime_error const& err) {
            connection_handler->write(Response(Error{500, err.what()}, dispatcher.getErrorBodyGenerator()).serialize());
            protocol_change = nullptr; // this means no further data will be passes to this connection_handler
        } catch (...) {
            connection_handler->write(Response(Error{500, "uncaught error"}, dispatcher.getErrorBodyGenerator()).serialize());
            protocol_change = nullptr; // this means no further data will be passes to this connection_handler
        }
    }
    return HttpProtocol::ConsumeResult{consumed, std::move(protocol_change)};
}

}
