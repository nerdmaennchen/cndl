#pragma once

#include "unique_function.h"

#include <simplyfile/Epoll.h>
#include <simplyfile/socket/Socket.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace cndl {

struct ProtocolHandler;
struct Dispatcher;

struct ConnectionHandler {
    using ClientSocket = simplyfile::ClientSocket;
    using Epoll = simplyfile::Epoll;
    using ByteBuf = std::vector<std::byte>;
    using ByteView = std::basic_string_view<std::byte>;
    using AfterSentCB = unique_func<void()>;
    using TransmitJob = std::tuple<ByteBuf, std::size_t, AfterSentCB>;

    ConnectionHandler(ClientSocket cs, Epoll& load_balancer, Dispatcher& dispatcher);
    ~ConnectionHandler();

    ConnectionHandler(ConnectionHandler&&) noexcept;
    ConnectionHandler& operator=(ConnectionHandler&&) noexcept;

    // IO is handled here
    void operator()(int flags);

    // try to send out_buf immediately and  enqueue the unsent remainder the write_queue of this connection handler
    // when the transmission is done on_after_sent will be called
    void write(ByteBuf out_buf, AfterSentCB on_after_sent={});

    void close(bool blocking); // closes the underlying socket and removes it from the IO loop

    size_t getOutBufferSize() const;

    Dispatcher& getDispatcher();

    Epoll& getIOLoop();
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};


}
