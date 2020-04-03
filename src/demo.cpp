#include <iostream>
#include <cstddef>
#include <algorithm>
#include <thread>

#include "sargparse/Parameter.h"
#include "sargparse/File.h"

#include "cndl/Request.h"
#include "cndl/Response.h"
#include "cndl/Error.h"
#include "cndl/Route.h"
#include "cndl/ConnectionHandler.h"

#include "cndl/Server.h"

#include "simplyfile/socket/Socket.h"
#include "simplyfile/socket/Host.h"
#include "simplyfile/Epoll.h"
#include "simplyfile/INotify.h"

#include "qrqma/template.h"


namespace 
{

void demo();
sargp::Task demo_task{demo};
auto optTemplPat = sargp::Parameter<sargp::Directory>("templates", "template_dir", "where to find the templates");

cndl::GlobalRoute globalRoute {"/global/?", [](cndl::Request const&) -> cndl::OptResponse {
    static qrqma::Template templ{qrqma::defaultLoader()("templates/global.html")};
    return templ();
}, {.methods={"GET", "POST"}}};

cndl::GlobalRoute template_route{"/(.*)?", [](cndl::Request const& request, std::string const& req_pat) -> cndl::OptResponse {
    static std::map<std::string, qrqma::Template> templates;
    static simplyfile::INotify inotify{IN_NONBLOCK};

    while (auto event = inotify.readEvent()) {
        templates.erase(event->path);
    }

    auto path = *optTemplPat / req_pat;
    auto nat_pat = path.native();
    auto it = templates.find(nat_pat);
    if (it == templates.end()) {
        try {
            auto loaded = qrqma::defaultLoader()(nat_pat);
            it = templates.emplace(nat_pat, loaded).first;
            inotify.watch(nat_pat, IN_CLOSE_WRITE);
        } catch (std::exception const& e) {
            return {};
        }
    }
    qrqma::symbol::UnorderedMultiMap url_args;
    qrqma::symbol::UnorderedMultiMap body_args;
    std::for_each(begin(request.header.url_args), end(request.header.url_args), [&](auto const& p) { url_args.emplace(p.first, p.second); });
    std::for_each(begin(request.header.body_args), end(request.header.body_args), [&](auto const& p) { body_args.emplace(p.first, p.second); });
    return (it->second)({
        {"url_args", url_args},
        {"body_args", body_args}
    });
}, {.methods={"GET", "POST"}}};

struct : cndl::WebsocketHandler {
    using Websocket = cndl::Websocket;
    using Request = cndl::Request;
    void onMessage([[maybe_unused]] Websocket& ws, [[maybe_unused]] AnyMessage msg) override {
        if (std::holds_alternative<TextMessage>(msg)) {
            auto txt = std::get<TextMessage>(msg);
            std::cout << txt << std::endl;
            if (txt == "close") {
                ws.close();
            }
            if (txt == "term") {
                cndl::Server::getGlobalServer().stop_looping();
            }
        }
        ws.send(msg);
    }

    bool onOpen([[maybe_unused]] Request const& request, [[maybe_unused]] Websocket& ws, [[maybe_unused]] int magic) {
        std::cout << "socket connected " << request.header.url << std::endl;
        using namespace std::literals::chrono_literals;
        ws.setAutoPing(1000ms, 100ms);
        return true;
    }

    void onClose([[maybe_unused]] Websocket& ws) override {
        std::cout << "socket closed" << std::endl;
    }

    void onPong([[maybe_unused]] Websocket& ws, [[maybe_unused]] BinMessage message) override {
        std::cout << "socket ponged" << std::endl;
    }
} echo_handler{};

void demo()
{
    cndl::Server& server = cndl::Server::getGlobalServer();
    server.listen(simplyfile::getHosts("localhost", "8080"));
    
    cndl::WSRoute wsroute{std::regex{R"(/test/(\d+)/)"}, echo_handler};
    server.getDispatcher().addRoute(&wsroute);

    server.loop_forever();
}

}
