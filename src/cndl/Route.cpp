#include "Route.h"

#include "Server.h"
#include "Dispatcher.h"

namespace cndl {

void registerRouteGlobally(RouteBase* route) {
    Server::getGlobalServer().getDispatcher().addRoute(route);
}

}
