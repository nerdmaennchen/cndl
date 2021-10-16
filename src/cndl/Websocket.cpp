#include "Websocket.h"

#include "ConnectionHandler.h"
#include "overloaded.h"

#include <cstdint>

namespace cndl {

constexpr std::byte operator"" _b(unsigned long long v){
    return std::byte(v);
}


Websocket::ConsumeResult Websocket::onDataReceived(ByteView bv) {
    int consumed = 0;
    while (true) {
        if (bv.size() < 6) { // min length of a message
            break;
        }

        // read the header
        bool mask = 0_b != (bv[1] & 0x80_b);
        if (not mask) {
            close(CloseCode::protocol_error, "received unmasked message");
            return Websocket::ConsumeResult{consumed, ProtocolChange{nullptr}};
        }

        bool fin = 0_b != (bv[0] & 0x80_b);
        OpCode opcode = OpCode{bv[0] & 0x0f_b};
        std::uint64_t payload_len = std::to_integer<int>(bv[1] & 0x7f_b);

        bv = bv.substr(2);
        int cur_consumed = 2;
        if (payload_len == 126) {
            payload_len = (std::to_integer<std::uint64_t>(bv[0])<<8) | (std::to_integer<std::uint64_t>(bv[1])<<0);
            bv = bv.substr(2);
            cur_consumed += 2;
        } else if (payload_len == 127) {
            payload_len = 0;
            for (int i{0}; i < 8; ++i) {
                payload_len = payload_len << 8 | std::to_integer<std::uint64_t>(bv[i]);
            }
            bv = bv.substr(8);
            cur_consumed += 8;
        }

        auto mask_key = bv.substr(0, 4);
        bv = bv.substr(4);
        cur_consumed += 4;

        if (bv.size() < payload_len) {
            break;
        }

        auto payload = bv.substr(0, payload_len);
        bv = bv.substr(payload_len);
        consumed += cur_consumed + payload_len;

        // unmask the payload
        ByteBuf unmasked{payload.size()};
        for (std::size_t i{0}; i < payload.size(); ++i) {
            unmasked[i] = payload[i] ^ mask_key[i % 4];
        }

        if (not fin) {
            if (opcode == OpCode::text or opcode == OpCode::binary) {
                if (frag_buffer.size()) {
                    close(CloseCode::protocol_error, "unterminated fragmented message");
                    return Websocket::ConsumeResult{consumed, ProtocolChange{nullptr}};
                }
                // start of a fragmented message
                cur_opcode = opcode;
            }
            std::copy(begin(unmasked), end(unmasked), std::back_inserter(frag_buffer));
        } else {
            if (opcode == OpCode::continuation) { // done reading fragmented message
                std::copy(begin(unmasked), end(unmasked), std::back_inserter(frag_buffer));
                if (cur_opcode == OpCode::text) {
                    handler->onMessage(*this, TextMessage{reinterpret_cast<char const*>(frag_buffer.data()), frag_buffer.size()});
                } else  {
                    handler->onMessage(*this, BinMessage{frag_buffer.data(), frag_buffer.size()});
                }
                frag_buffer.clear();
            } else if (opcode ==  OpCode::text) {
                    handler->onMessage(*this, TextMessage{reinterpret_cast<char const*>(unmasked.data()), unmasked.size()});
            } else if (opcode ==  OpCode::binary) {
                    handler->onMessage(*this, BinMessage{unmasked.data(), unmasked.size()});
            } else if (opcode ==  OpCode::close) {
                    close(CloseCode::normal, "thx goodbye");
                return Websocket::ConsumeResult{consumed, ProtocolChange{nullptr}};
            } else if (opcode ==  OpCode::ping) {
                handler->onPing(*this, BinMessage{unmasked.data(), unmasked.size()});
            } else if (opcode ==  OpCode::pong) {
                if (auto_ping) {
                    std::lock_guard lock{auto_ping->mutex};
                    auto_ping->timeout_timer.cancel();
                }
                handler->onPong(*this, BinMessage{unmasked.data(), unmasked.size()});
            }
        }
    }
    return Websocket::ConsumeResult{consumed, ProtocolChange{}};
}

void Websocket::onPeerClose() {
    handler->onClose(*this);
    if (auto_ping) {
        auto& loop = connection_handler->getIOLoop();
        loop.rmFD(auto_ping->ping_timer, false);
        loop.rmFD(auto_ping->timeout_timer, false);
    }
}

void Websocket::send(BinMessage message, OpCode opcode, bool fin, AfterSentCB on_after_sent) {
    ByteBuf serialized{};
    serialized.reserve(message.size() + 2 + 8); // reserve enough space for the header and the optional extra payload_len fields

    serialized.emplace_back(std::byte(static_cast<unsigned char>(opcode) | (fin?0x80:0x00)));

    std::uint64_t pl = message.size();
    if (pl <= 125) {
        serialized.emplace_back(std::byte(pl));
    } else {
        if (message.size() <= 0xffff) {
            serialized.emplace_back(std::byte(126));
            serialized.emplace_back(std::byte(pl >> 8));
            serialized.emplace_back(std::byte(pl >> 0));
        } else {
            serialized.emplace_back(std::byte(127));
            for (auto i{7}; i >= 0; --i) {
                serialized.emplace_back(std::byte(pl >> (i*8)));
            }
        }
    }
    std::copy(begin(message), end(message), std::back_inserter(serialized));
    connection_handler->write(std::move(serialized), std::move(on_after_sent));
}

void Websocket::send(AnyMessage message, AfterSentCB on_after_sent) {
    std::visit(detail::overloaded {
        [&, this](TextMessage msg) {
            send({reinterpret_cast<std::byte const*>(msg.data()), msg.size()}, OpCode::text, true, std::move(on_after_sent));
        },
        [&, this](BinMessage msg) {
            send(msg, OpCode::binary, true, std::move(on_after_sent));
        }
    }, message);
}

void Websocket::ping(AnyMessage message, AfterSentCB on_after_sent) {
    std::visit([this, &on_after_sent](auto msg) {
        if (msg.size() > 125) {
            throw std::invalid_argument("ping payload too long");
        }
        auto binmsg = BinMessage{reinterpret_cast<std::byte const*>(msg.data()), msg.size()};
        send(binmsg, OpCode::ping, true, std::move(on_after_sent));
    }, message);
}

void Websocket::close(CloseCode code,AnyMessage reason) {
    BinMessage bm = std::visit(detail::overloaded{
        [](BinMessage msg) {
            if (msg.size() > 125) {
                throw std::invalid_argument("close payload too long");
            }
            return msg;
        },
        [](TextMessage msg) {
            if (msg.size() > 125) {
                throw std::invalid_argument("close payload too long");
            }
            return BinMessage{reinterpret_cast<std::byte const*>(msg.data()), msg.size()};
    }}, reason);

    if (auto_ping) {
        auto& loop = connection_handler->getIOLoop();
        std::lock_guard lock{auto_ping->mutex};
        loop.rmFD(auto_ping->ping_timer, false);
        loop.rmFD(auto_ping->timeout_timer, false);
    }

    std::vector<std::byte> payload;
    payload.reserve(bm.size() + 2);
    payload.emplace_back(std::byte((static_cast<uint16_t>(code) >> 8) & 0xff));
    payload.emplace_back(std::byte((static_cast<uint16_t>(code) >> 0) & 0xff));
    std::copy(begin(bm), end(bm), std::back_inserter(payload));
    handler->onClose(*this);
    send({payload.data(), payload.size()}, OpCode::close, true, [ch=connection_handler]{
        ch->close(false);
    });
}

void Websocket::setAutoPing(std::chrono::milliseconds ping_interval, std::chrono::milliseconds timeout) {
    auto& loop = connection_handler->getIOLoop();
    if (auto_ping) {
        std::lock_guard lock{auto_ping->mutex};
        loop.rmFD(auto_ping->ping_timer, false);
        loop.rmFD(auto_ping->timeout_timer, false);
    }
    auto_ping.emplace();

    std::lock_guard lock{auto_ping->mutex};
    auto_ping->ping_timer.reset(std::chrono::duration_cast<PingTimers::Duration>(ping_interval));

    loop.addFD(auto_ping->timeout_timer, [this](int){
        close(CloseCode::normal, "ping timeout");
    }, EPOLLIN|EPOLLET);

    loop.addFD(auto_ping->ping_timer, [this, timeout](int){
        ping("ping?", [this, timeout]{
            auto_ping->timeout_timer.reset(timeout, true);
        });
        auto_ping->ping_timer.getElapsed(); // clear the ping timer
    }, EPOLLIN|EPOLLET);
}


void WebsocketHandler::onPong(Websocket&, BinMessage) {
    // do nothing
}

void WebsocketHandler::onPing(Websocket& ws, BinMessage message) {
    ws.send(message, Websocket::OpCode::pong);
}

}
