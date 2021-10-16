#include "ConnectionHandler.h"

#include "HttpProtocol.h"
#include "ProtocolHandler.h"

#include <atomic>
#include <mutex>
#include <sys/epoll.h>

namespace cndl {
namespace {
bool flush_job(ConnectionHandler::TransmitJob &job, ConnectionHandler::ClientSocket const& con) {
    auto& [out_buf, bytes_sent, cb] = job;
    while (bytes_sent < out_buf.size()) {
        int w = ::send(con, out_buf.data()+bytes_sent, out_buf.size()-bytes_sent, MSG_NOSIGNAL);
        if (w <= 0 and (errno == EWOULDBLOCK || errno == EAGAIN)) {
            return false;
        }
        bytes_sent += w;
    }
    if (cb) {
        cb();
    }
    return true;
}
}

struct ConnectionHandler::Pimpl {
    ClientSocket con;
    Epoll& epoll;
    Dispatcher& dispatcher;

    ByteBuf in_buf;
    std::vector<TransmitJob> transmit_jobs;
    std::recursive_mutex transmit_job_mutex;

    std::unique_ptr<ProtocolHandler> protocol{};

    std::atomic<size_t> outBufferSize{0};

    Pimpl(ClientSocket i_con, Epoll& i_epoll, Dispatcher& i_dispatcher, ConnectionHandler* i_handler)
      : con{std::move(i_con)}
      , epoll{i_epoll}
      , dispatcher{i_dispatcher}
      , protocol{std::make_unique<HttpProtocol>(i_handler)}
    {}

    void write(ByteBuf out_buf, AfterSentCB on_after_sent) {
        auto job = TransmitJob(std::move(out_buf), 0U, std::move(on_after_sent));
        std::lock_guard lock{transmit_job_mutex};
        if (not transmit_jobs.empty() || not flush_job(job, con)) {
            // update buffer size
            auto const& [out_buf, bytes_sent, cb] = job;
            outBufferSize += out_buf.size() - bytes_sent;

            // queue job
            transmit_jobs.emplace_back(std::move(job));
            // send when the socket is ready
            epoll.modFD(con, EPOLLIN|EPOLLOUT|EPOLLHUP|EPOLLRDHUP|EPOLLONESHOT);
        }
    }

    void operator()(int flags) {
        if (flags & (EPOLLERR|EPOLLHUP|EPOLLRDHUP)) {
            if (protocol) {
                protocol->onPeerClose();
            }
            close(false);
            return;
        }
        int mod_flags = EPOLLIN|EPOLLHUP|EPOLLRDHUP|EPOLLONESHOT;

        if ((flags & EPOLLIN) and protocol) {
            while (true) {
                constexpr int read_size = 4096;
                auto head = in_buf.size();
                in_buf.resize(head+read_size);
                int r = ::recv(con, in_buf.data()+head, read_size, 0);
                if (r < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
                    in_buf.resize(head);
                    break;
                }
                in_buf.resize(head+r);
                if (r < read_size) {
                    break;
                }
            }
            auto [consumed, prot_change] = protocol->onDataReceived(ProtocolHandler::ByteView{in_buf.data(), in_buf.size()});
            in_buf.erase(begin(in_buf), begin(in_buf)+consumed);

            if (prot_change) {
                protocol = std::move(*prot_change);
            }
        }

        if (flags & EPOLLOUT) {
            std::lock_guard lock{transmit_job_mutex};
            bool sent_something = false;
            while (not transmit_jobs.empty()) {
                auto job = transmit_jobs.begin();

                // update buffer size and flush
                auto to_send = std::get<0>(*job).size() - std::get<1>(*job);
                bool job_sent = flush_job(*job, con);
                auto remaining = std::get<0>(*job).size() - std::get<1>(*job);

                outBufferSize -= to_send - remaining;

                sent_something |= job_sent;
                if (not job_sent) {
                    break;
                }
                transmit_jobs.erase(job);
            }

            // if there is nothing to send and no protocol to listen (i.e., when we have flushed all data) bail out
            // the protocol could change during the handling of a callback
            if (not protocol) { 
                close(false);
            }
        }

        {
            std::lock_guard lock{transmit_job_mutex};
            if (not transmit_jobs.empty()) {
                mod_flags |= EPOLLOUT;
            }
        }

        if (con.valid()) {
            // up to here con might have been closed (and thus deregistered from epoll)
            // we only need to rearm the epoll handle if the connection is still open
            epoll.modFD(con, mod_flags);
        }
    }

    void close(bool blocking) {
        epoll.rmFD(con, blocking);
        con.close();
    }

    size_t getOutBufferSize() const {
        return outBufferSize;
    }

    Dispatcher& getDispatcher() {
        return dispatcher;
    }

    Epoll& getIOLoop() {
        return epoll;
    }
};

void ConnectionHandler::write(ByteBuf out_buf, AfterSentCB on_after_sent) {
    pimpl->write(std::move(out_buf), std::move(on_after_sent));
}

void ConnectionHandler::close(bool blocking) {
    pimpl->close(blocking);
}
size_t ConnectionHandler::getOutBufferSize() const {
    return pimpl->getOutBufferSize();
}


Dispatcher& ConnectionHandler::getDispatcher() {
    return pimpl->getDispatcher();
}

ConnectionHandler::Epoll& ConnectionHandler::getIOLoop() {
    return pimpl->getIOLoop();
}

void ConnectionHandler::operator()(int flags) {
    (*pimpl)(flags);
}

ConnectionHandler::ConnectionHandler(ClientSocket cs, Epoll& load_balancer, Dispatcher& dispatcher)
  : pimpl{std::make_unique<Pimpl>(std::move(cs), load_balancer, dispatcher, this)}
{}

ConnectionHandler::ConnectionHandler(ConnectionHandler&& rhs) noexcept
  : pimpl{std::move(rhs.pimpl)}
{
    if (pimpl->protocol) {
        pimpl->protocol->setConnectionHandler(this);
    }
}

ConnectionHandler& ConnectionHandler::operator=(ConnectionHandler&& rhs) noexcept {
    std::swap(this->pimpl, rhs.pimpl);
    if (pimpl->protocol) {
        pimpl->protocol->setConnectionHandler(this);
    }
    return *this;
}
ConnectionHandler::~ConnectionHandler()
{}

}
