#pragma once

#include "Dispatcher.h"

#include <simplyfile/Epoll.h>
#include <simplyfile/socket/Host.h>

#include <mutex>

namespace cndl {

struct Server : simplyfile::Epoll {
    Server(std::vector<simplyfile::Host> const& hosts={});
    ~Server();
    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;

    void listen(std::vector<simplyfile::Host> const& hosts);

    // calls work() in a loop until stop_looping() is called
    void loop_forever();
    // stops the loop (can be called from a handler of this server)
    void stop_looping();
    // stops the loop and wait for the loop's termination (cannot be called from a handler of this server)
    void stop_looping_sync();

    Dispatcher& getDispatcher();

    // returns a singleton for applications where the routing shall be done globally
    static Server& getGlobalServer();
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}
