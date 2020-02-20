#pragma once

#include <vector>
#include <cstddef>
#include <string_view>
#include <optional>
#include <memory>

namespace cndl
{

struct ConnectionHandler;

struct ProtocolHandler {
    using ByteBuf = std::vector<std::byte>;
    using ByteView = std::basic_string_view<std::byte>;
    using ProtocolChange = std::optional<std::unique_ptr<ProtocolHandler>>;
    using ConsumeResult = std::pair<int, ProtocolChange>;

    ProtocolHandler(ConnectionHandler* handler) : connection_handler{handler} {}
    virtual ~ProtocolHandler() = default;
    
    // called when data was received
    // return the amount of bytes consumed
    virtual ConsumeResult onDataReceived(ByteView received) = 0;
    // called when the remote hung up
    virtual void onPeerClose() {}

    void setConnectionHandler(ConnectionHandler* new_handler) {
        connection_handler = new_handler;
    }
protected:
    ConnectionHandler* connection_handler;
};

}