#include "print_statement.h"
#include "convert.h"

#include "../overloaded.h"

#include <typeindex>

namespace qrqma {
namespace actions {

void action<grammar::print_expression>::apply(ContextP &context) {
        auto stack = context->popAllExpressions();
        if (stack.empty()) {
            return;
        }
        if (stack.size() != 1) {
            throw std::runtime_error("internal error; there are too many expressions on the expression stack!");
        }
        auto expression = std::move(stack.back());
        std::visit(detail::overloaded{
            [context=context.get()] (types::ConstantExpression const& ce) {
                context->addRenderToken(std::move(convert<std::string>(ce.eval())));
            },
            [context=context.get()] (types::NonconstantExpression& nce) {
                context->addRenderToken([nce=std::move(nce), &context]() -> Context::RenderOutput  {
                    return {
                        convert<std::string>(nce.eval()),
                        false
                    };
                });
            }
        }, expression);
    }

}
}