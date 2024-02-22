#include "Dispatcher.h"

#include <mutex>
#include <vector>
#include <ranges>

namespace cndl {

struct Dispatcher::Pimpl {
    ErrorBodyGenerator error_body_generator;
    // method -> handler
    std::vector<RouteBase*> routes;
    std::mutex routes_mutex;

    std::vector<WSRouteBase*> ws_routes;
    std::mutex ws_mutex;

    Pimpl(ErrorBodyGenerator generator)
      : error_body_generator{std::move(generator)}
    {}
};

Response Dispatcher::route(Request const& request) noexcept {
    std::lock_guard lock{pimpl->routes_mutex};
    struct URLMatchInfo {
        std::cmatch match{};
        RouteBase* route{};
        int score{};
    };
    std::vector<URLMatchInfo> match_infos;

    for (auto* r : pimpl->routes) {
        auto match = r->match(request);
        if (not match) {
            continue;
        }
        int score = request.header.resource.size();
        for (auto sub_m : *match | std::views::drop(1)) {
            score -= sub_m.length();
        }
        match_infos.emplace_back(URLMatchInfo{
            .match = *match,
            .route = r,
            .score = score
        });
    }

    auto best_route = std::max_element(begin(match_infos), end(match_infos), [](auto const& l, auto const& r) { return l.score < r.score; });
    if (best_route == match_infos.end()) {
        return Response{404, pimpl->error_body_generator};
    }
    try {
        auto resp = (*best_route->route)(request, best_route->match);
        if (resp) {
            return *resp;
        }
    } catch (Error const& err) {
        return Response(err, pimpl->error_body_generator);
    } catch (std::exception const& err) {
        return Error{500, err.what()};
    } catch (...) {
        return Error{500, "uncaught error"};
    }
    return Response{404, pimpl->error_body_generator};
}

WSRouteBase& Dispatcher::routeWS(Request const& request) {
    std::lock_guard lock{pimpl->ws_mutex};
    for (auto* r : pimpl->ws_routes) {
        auto accept = r->canOpen(request);
        if (accept) {
            return *r;
        }
    }
    throw Error{404};
}

void Dispatcher::addRoute(RouteBase& r) {
    std::lock_guard lock{pimpl->routes_mutex};
    auto& routes = pimpl->routes;
    routes.emplace_back(&r);
}

void Dispatcher::removeRoute(RouteBase& r) {
    std::lock_guard lock{pimpl->routes_mutex};
    auto& routes = pimpl->routes;
    routes.erase(std::find(begin(routes), end(routes), &r));
}

void Dispatcher::addRoute(WSRouteBase& r) {
    std::lock_guard lock{pimpl->ws_mutex};
    auto& routes = pimpl->ws_routes;
    routes.emplace_back(&r);
}

void Dispatcher::removeRoute(WSRouteBase& r) {
    std::lock_guard lock{pimpl->ws_mutex};
    auto& routes = pimpl->ws_routes;
    routes.erase(std::find(begin(routes), end(routes), &r));
}

Dispatcher::ErrorBodyGenerator const& Dispatcher::getErrorBodyGenerator() const {
    return pimpl->error_body_generator;
}

Dispatcher::Dispatcher(ErrorBodyGenerator generator) 
    : pimpl{std::make_unique<Pimpl>(std::move(generator))}
{}

Dispatcher::Dispatcher(Dispatcher&& other) noexcept {
    *this = std::move(other);
}

Dispatcher& Dispatcher::operator=(Dispatcher&& other) noexcept {
    std::swap(pimpl, other.pimpl);
    return *this;
}

Dispatcher::~Dispatcher() {}

}
