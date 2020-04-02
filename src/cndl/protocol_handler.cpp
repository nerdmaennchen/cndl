#include "protocol_handler.h"

#include "connection_handler.h"

namespace cndl
{
size_t ProtocolHandler::getOutBufferSize() const {
    return connection_handler->getOutBufferSize();
}

}
