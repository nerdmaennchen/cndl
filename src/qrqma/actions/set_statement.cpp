#include "set_statement.h"
#include "types.h"

#include "../symbol.h"

#include "../overloaded.h"

namespace qrqma {
namespace actions {

void action<grammar::set_control_statement>::success(std::string &symbol_name, ContextP& context) {
    auto e   = context->popExpression();

    context->setSymbol(symbol_name, std::visit(detail::overloaded{
        [context=context.get()] (types::ConstantExpression const& ce) -> types::Expression {
            return types::ConstantExpression{
                [val = ce.eval()]{ return val; }
            };
        },
        [context=context.get(), symbol_name] (types::NonconstantExpression& nce) -> types::Expression{
            return std::move(nce);
        },
    }, e));
    context->addRenderToken([context=context.get(), sym=(*context)[symbol_name], symbol_name]() -> symbol::RenderOutput {
        context->setSymbol(symbol_name, sym);
        return {"", false};
    });
}

}
}