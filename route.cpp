#include "route.h"

#include "server.h"
#include "dispatcher.h"

namespace cndl
{

void registerRouteGlobally(RouteBase* route) {
    Server::getGlobalServer().getDispatcher().addRoute(route);
}

}
