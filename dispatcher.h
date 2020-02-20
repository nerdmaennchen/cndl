#pragma once

#include "route.h"
#include "ws_route.h"
#include "request.h"
#include "response.h"
#include "websocket.h"

namespace cndl
{

struct Dispatcher {
    using ErrorBodyGenerator = Response::ErrorBodyGenerator;

    Dispatcher(ErrorBodyGenerator generator={});
    ~Dispatcher();

    Response route(Request const& request) noexcept;
    WebsocketHandler* routeWS(Request const& request, Websocket& ws);

    void addRoute(RouteBase* r);
    void removeRoute(RouteBase* r);

    void addRoute(WSRouteBase* r);
    void removeRoute(WSRouteBase* r);

    ErrorBodyGenerator const& getErrorBodyGenerator() const;
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

}