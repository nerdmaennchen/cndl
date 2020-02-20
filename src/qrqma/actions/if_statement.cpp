#include "if_statement.h"
#include "convert.h"

#include "../overloaded.h"

namespace qrqma {
namespace actions {

void action<grammar::if_control_statement>::success(ContextP& outerC, ContextP& ifC, ContextP& elseC) {
    auto e = outerC->popExpression();

    outerC->addRenderToken(std::visit(detail::overloaded{
        [&ifC, &elseC, &outerC] (types::ConstantExpression const& ce) -> Context::RenderToken {
            if (convert<types::Bool>(ce.eval())) {
                auto const& symbols = ifC->getSymbolTable();
                for (auto const&[k, v] : symbols) {
                    outerC->setSymbol(k, v);
                }
                return [ifC=std::move(ifC)]() -> symbol::RenderOutput {
                    return (*ifC)();
                };
            } else {
                auto const& symbols = elseC->getSymbolTable();
                for (auto const& [k, v] : symbols) {
                    outerC->setSymbol(k, v);
                }
                return [elseC=std::move(elseC), outerC=outerC.get()]() -> symbol::RenderOutput {
                    return (*elseC)();
                };
            }
        },
        [&ifC, &elseC, &outerC] (types::NonconstantExpression& nce) -> Context::RenderToken {
            return [ifC=std::move(ifC), elseC=std::move(elseC), outerC=outerC.get(), nce=std::move(nce)]() -> symbol::RenderOutput {
                if (convert<types::Bool>(nce.eval())) {
                    return (*ifC)();
                } else {
                    return (*elseC)();
                }
            };
        }
    }, e));
}

}
}