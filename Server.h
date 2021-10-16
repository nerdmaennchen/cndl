#pragma once

#include "Dispatcher.h"

#include <simplyfile/Epoll.h>
#include <simplyfile/socket/Host.h>

#include <mutex>

namespace cndl {

struct Server {
    Server(simplyfile::Host const& host, simplyfile::Epoll& epoll, int backlog=0);
    Server(simplyfile::Epoll& epoll);
    ~Server();
    Server(Server&&) noexcept;
    Server& operator=(Server&&) noexcept;

    void listen(simplyfile::Host const& host, int backlog=0);

    Dispatcher& getDispatcher();

    simplyfile::Epoll& getEpoll();

    // returns a singleton for applications where the routing shall be done globally
    static Server& getGlobalServer();

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}
