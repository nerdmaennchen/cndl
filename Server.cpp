#include "Server.h"

#include "ConnectionHandler.h"

#include <simplyfile/Event.h>

#include <atomic>
#include <list>

namespace cndl {

struct Server::Pimpl {
    Dispatcher dispatcher;

    std::list<simplyfile::ServerSocket> server_sockets;

    simplyfile::Epoll& m_epoll;
    Pimpl(simplyfile::Epoll& epoll) : m_epoll{epoll} {
    }

    ~Pimpl() {
        for (auto& ss : server_sockets) {
            m_epoll.rmFD(ss, true);
        }
    }

    void listen(simplyfile::Host const& host, int backlog) {
        auto& ss = server_sockets.emplace_back(host);
        ss.setFlags(O_NONBLOCK);
        m_epoll.addFD(ss, [this, &ss](int flags) {
            if (flags != EPOLLIN) {
                m_epoll.rmFD(ss, false);
                return;
            }
            while (true) {
                auto client = ss.accept();
                if (not client.valid()) {
                    break;
                }
                client.setFlags(O_NONBLOCK);

                int fd = client;
                m_epoll.addFD(fd, ConnectionHandler{std::move(client), m_epoll, dispatcher}, EPOLLIN|EPOLLHUP|EPOLLRDHUP|EPOLLONESHOT, "cndl::io");
            }
            m_epoll.modFD(ss, EPOLLIN|EPOLLONESHOT);
        }, EPOLLIN|EPOLLONESHOT, "cndl::accept");
        ss.listen(backlog);
    }
};

Dispatcher& Server::getDispatcher() {
    return pimpl->dispatcher;
}

void Server::listen(simplyfile::Host const& host, int backlog) {
    pimpl->listen(host, backlog);
}

Server::Server(simplyfile::Epoll& epoll) : pimpl{std::make_unique<Pimpl>(epoll)} {
}

Server::Server(simplyfile::Host const& host, simplyfile::Epoll& epoll, int backlog) : pimpl{std::make_unique<Pimpl>(epoll)} {
    pimpl->listen(host, backlog);
}

Server::~Server() {
}

Server::Server(Server&& rhs) noexcept  : pimpl{std::move(rhs.pimpl)} {}
Server& Server::operator=(Server&& rhs) noexcept {
    std::swap(this->pimpl, rhs.pimpl);
    return *this;
}


Server& Server::getGlobalServer() {
    static Server global_instance{getGlobalEpoll()};
    return global_instance;
}

simplyfile::Epoll& Server::getGlobalEpoll() {
    static simplyfile::Epoll global_epoll;
    return global_epoll;
}



}
