#include "ProtocolHandler.h"

#include "ConnectionHandler.h"

namespace cndl {

size_t ProtocolHandler::getOutBufferSize() const {
    return connection_handler->getOutBufferSize();
}

}
