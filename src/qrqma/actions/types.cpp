#include "types.h"
#include "../overloaded.h"

namespace qrqma {
namespace actions {
namespace types {

NonconstantExpression asNonconstantE(Expression&& e) {
    return std::visit(
        [](auto&& e) -> NonconstantExpression {
            return std::move(e);
        }
    , std::move(e));
}

}
}
}