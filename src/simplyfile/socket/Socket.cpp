#include "Socket.h"

#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <unistd.h>
#include <sys/un.h>

namespace simplyfile
{
ClientSocket::ClientSocket(int _fd, Host const& _host)
	: FileDescriptor(_fd)
	, host(_host)
{}

ClientSocket::ClientSocket(Host const& _host)
	: FileDescriptor(::socket(_host.family, _host.socktype, _host.protocol))
	, host(_host)
{}

void ClientSocket::connect() {
	if (::connect(*this, reinterpret_cast<struct sockaddr const*>(&host.sockaddr), host.sockaddrLen) != 0) {
		throw std::runtime_error("cannot connect to host: " + host.getName() + " errno: " + std::string(strerror(errno)));
	}
}

int ClientSocket::getBytesAvailable() const {
	int bytesAvailable;
	if (ioctl(*this, FIONREAD, &bytesAvailable)) {
		throw std::runtime_error("cannot determine how many bytes are available in socket");
	}
	return bytesAvailable;
}

ServerSocket::ServerSocket(Host const& _host, bool reusePort)
	: host(_host)
{
	FileDescriptor sock = ::socket(host.family, host.socktype, host.protocol);
	int optVal = reusePort;
    ::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optVal, sizeof(optVal));
	if (not sock.valid()) {
		throw std::runtime_error("cannot create socket " + std::string(strerror(errno)));
	}
	if (host.family == AF_LOCAL) {
		// for unix sockets we might need to unlink the socket-file that is being created
		struct sockaddr_un& address = reinterpret_cast<struct sockaddr_un&>(host.sockaddr);
		::unlink(address.sun_path);
	}
	if (::bind(sock, reinterpret_cast<struct sockaddr const*>(&host.sockaddr), host.sockaddrLen)) {
		throw std::runtime_error("cannot bind socket to: " + host.getName());
	}
	FileDescriptor::operator =(std::move(sock));
}

ServerSocket::~ServerSocket() {
	if (valid()) {
		if (host.family == AF_LOCAL) {
			// for unix sockets we might need to unlink the socket-file that is being created
			struct sockaddr_un& address = reinterpret_cast<struct sockaddr_un&>(host.sockaddr);
			::unlink(address.sun_path);
		}
	}
}

ClientSocket ServerSocket::accept() const {
	Host h = host;
	int _fd = ::accept(*this, reinterpret_cast<struct sockaddr*>(&h.sockaddr), &h.sockaddrLen);
	return ClientSocket {_fd, h};
}

void ServerSocket::listen(int backlog) {
	if (::listen(*this, backlog)) {
		throw std::runtime_error("cannot listen on socket " + std::string(strerror(errno)));
	}
}

}
