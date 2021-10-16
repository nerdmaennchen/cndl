#pragma once

#include "Request.h"
#include "Response.h"
#include "Route.h"
#include "Websocket.h"
#include "WSRoute.h"

namespace cndl {

struct Dispatcher {
    using ErrorBodyGenerator = Response::ErrorBodyGenerator;

    Dispatcher(ErrorBodyGenerator generator={});
    Dispatcher(Dispatcher&&) noexcept;
    Dispatcher& operator=(Dispatcher&&) noexcept;
    ~Dispatcher();

    Response route(Request const& request) noexcept;
    WSRouteBase& routeWS(Request const& request);

    void addRoute(RouteBase& r);
    void removeRoute(RouteBase& r);

    void addRoute(WSRouteBase& r);
    void removeRoute(WSRouteBase& r);

    ErrorBodyGenerator const& getErrorBodyGenerator() const;
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}
