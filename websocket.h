#pragma once

#include "protocol_handler.h"
#include "connection_handler.h"
#include "simplyfile/Timer.h"

#include <cstddef>
#include <string_view>
#include <variant>
#include <chrono>
#include <mutex>

namespace cndl
{
struct WebsocketHandler;

struct Websocket : ProtocolHandler {
    using ByteView = ProtocolHandler::ByteView;
    using ByteBuf  = ProtocolHandler::ByteBuf;
    using ProtocolChange = ProtocolHandler::ProtocolChange;
    using ConsumeResult  = ProtocolHandler::ConsumeResult;
    using AfterSentCB    = ConnectionHandler::AfterSentCB;

    using TextMessage = std::string_view;
    using BinMessage  = std::basic_string_view<std::byte>;
    using AnyMessage  = std::variant<TextMessage, BinMessage>;

    enum class OpCode : std::uint8_t {
        continuation = 0x00,
        text         = 0x01,
        binary       = 0x02,
        close        = 0x08,
        ping         = 0x09,
        pong         = 0x0a,
    };
    enum class CloseCode : std::uint16_t {
        normal           = 1000,
        going_away       = 1001,
        protocol_error   = 1002,
        unacceptable     = 1003,
        invalid_data     = 1007,
        policy_violation = 1008,
        message_too_big  = 1009,
        unexpected_error = 1011
    };


    using ProtocolHandler::ProtocolHandler;
    
    virtual ~Websocket() = default;
    
    // called from the IO loop
    ConsumeResult onDataReceived(ByteView received) override;
    void onPeerClose() override;
    
    // called from the application 
    void send(AnyMessage message);
    void ping(AnyMessage message, AfterSentCB on_after_sent={}); // max payload length is 125

    void close(CloseCode code=CloseCode::normal, AnyMessage reason="");

    void setHandler(WebsocketHandler* new_handler) {
        handler = new_handler;
    }
    WebsocketHandler* getHandler() {
        return handler;
    }

    void send(BinMessage message, OpCode opcode, bool fin=true, AfterSentCB on_after_sent={});

    /*
     * setup auto ping (a ping will be auto scheduled on the io handler of this socket's connection_handler)
     * ping_interval: how often to send a ping
     * timeout: how long to wait for a timeout before considering this connection broken
     */
    void setAutoPing(std::chrono::milliseconds ping_interval, std::chrono::milliseconds timeout);
private:
    WebsocketHandler* handler{nullptr};
    std::vector<std::byte> frag_buffer;
    OpCode cur_opcode{};

    struct PingTimers {
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::high_resolution_clock::time_point;
        using Duration = TimePoint::duration;

        simplyfile::Timer ping_timer{};
        simplyfile::Timer timeout_timer{};

        Duration timeout{};
        TimePoint last_ping_ts{};
        TimePoint last_pong_ts{};

        std::mutex mutex{};
    };
    std::optional<PingTimers> auto_ping;
};

struct WebsocketHandler {
    using TextMessage = Websocket::TextMessage;
    using BinMessage  = Websocket::BinMessage;
    using AnyMessage  = Websocket::AnyMessage;
    virtual ~WebsocketHandler() = default;

    // to be implemented by the endpoint
    virtual void onMessage(Websocket& ws, AnyMessage message) = 0;
    virtual void onClose([[maybe_unused]] Websocket& ws) {}

    // you have to implement an onOpen method that accepts the parameters passed from the URL:
    // bool onOpen(Request const&, Websocket&, urlargs...)
    // onOpen returns true if the incoming socket shall be accepted

    virtual void onPing(Websocket& ws, BinMessage message);
    virtual void onPong(Websocket& ws, BinMessage message);
};



}
