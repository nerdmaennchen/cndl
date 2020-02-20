#include "grammar.h"

namespace qrqma {
namespace actions {

void action<grammar::grammar>::success(ContextP& inner_context, Context& outer_context) {
    outer_context.addRenderToken([inner_context=std::move(inner_context)]{
        return (*inner_context)();
    });
}

}
}