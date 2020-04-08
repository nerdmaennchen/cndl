#include "Server.h"

#include "ConnectionHandler.h"

#include <simplyfile/Event.h>

#include <atomic>
#include <list>

namespace cndl {

struct Server::Pimpl {
    Dispatcher dispatcher;

    std::list<simplyfile::ServerSocket> server_sockets;

    simplyfile::Event stopFD;
    std::atomic_bool running{false};

    std::mutex stop_mutex;
    std::condition_variable stop_cv;

    Epoll& m_epoll;
    Pimpl(Epoll& epoll) : m_epoll{epoll} {
        m_epoll.addFD(stopFD, [=](int){ stopFD.get(); }, EPOLLIN|EPOLLET);
    }

    ~Pimpl() {
        m_epoll.rmFD(stopFD, true);
        for (auto& ss : server_sockets) {
            m_epoll.rmFD(ss, true);
        }
    }

    void listen(std::vector<simplyfile::Host> const& hosts) {
        for (auto const& host : hosts) {
            auto& ss = server_sockets.emplace_back(host);
            ss.setFlags(O_NONBLOCK);
            m_epoll.addFD(ss, [=, &ss](int flags) {
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
                    m_epoll.addFD(fd, ConnectionHandler{std::move(client), m_epoll, dispatcher}, EPOLLIN|EPOLLHUP|EPOLLRDHUP|EPOLLONESHOT);
                }
                m_epoll.modFD(ss, EPOLLIN|EPOLLONESHOT);
            }, EPOLLIN|EPOLLONESHOT);
            ss.listen();
        }
    }
};

// stops the loop
void Server::stop_looping() {
    pimpl->running = false;
}

void Server::stop_looping_sync() {
    if (not pimpl->running){
        return;
    }
    std::unique_lock l{pimpl->stop_mutex};
    pimpl->running = false;
    pimpl->stopFD.put(1);
    pimpl->stop_cv.wait(l);
}

Dispatcher& Server::getDispatcher() {
    return pimpl->dispatcher;
}

void Server::loop_forever() {
    pimpl->running = true;
    while (pimpl->running) {
        work();
    }
    std::lock_guard l{pimpl->stop_mutex};
    pimpl->stop_cv.notify_all();
}

void Server::listen(std::vector<simplyfile::Host> const& hosts) {
    pimpl->listen(hosts);
}

Server::Server(std::vector<simplyfile::Host> const& hosts) : pimpl{std::make_unique<Pimpl>(*this)} {
    pimpl->listen(hosts);
}

Server::~Server() {
    stop_looping_sync();
}

Server::Server(Server&& rhs) noexcept  : pimpl{std::move(rhs.pimpl)} {}
Server& Server::operator=(Server&& rhs) noexcept {
    std::swap(this->pimpl, rhs.pimpl);
    return *this;
}


Server& Server::getGlobalServer() {
    static Server global_instance;
    return global_instance;
}


}
