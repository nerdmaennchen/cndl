#include "extends_statement.h"

#include "types.h"
#include "../grammar/grammar.h"
#include "actions.h"
#include "../demangle.h"
#include "../overloaded.h"

namespace qrqma {
namespace actions {

namespace pegtl = tao::pegtl;

void action<grammar::extends_control_statement>::apply(ContextP& context) {
    auto name = std::visit(detail::overloaded{
        [] (types::ConstantExpression const& ce) -> std::string {
            return std::any_cast<std::string>(ce.eval());
        },
        [] (auto const& other) -> std::string {
            throw std::runtime_error("cannot use a " + internal::demangle(typeid(other)) + " as extends specifier!");
        }
    }, context->popExpression());
    
    context->addRenderToken([base_ctx = std::optional<Context>{}, name=std::move(name), ctx=context.get()]() mutable -> Context::RenderOutput {
        if (not base_ctx) {
            auto loader      = ctx->getTemplateLoader();
            
            auto loaderCtx = ctx;
            while (loaderCtx and not loader) {
                loader = loaderCtx->getTemplateLoader();
                loaderCtx = loaderCtx->getParentContext();
            }

            if (not loader) {
                throw std::runtime_error{"cannot extend a template without specifying a template loader!"};
            }

            base_ctx.emplace(ctx);
            auto content = loader(name);
            pegtl::parse<pegtl::if_must<grammar::grammar, pegtl::eof>, actions::action>(
                pegtl::memory_input{content, ""}, 
                *base_ctx
            );
            
        }

        return {(*base_ctx)().rendered, true};
    });
}

}
}