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
    
	
	auto loaderCtx = context.get();
	while (loaderCtx and not loaderCtx->getTemplateLoader()) {
		loaderCtx = loaderCtx->getParentContext();
	}
	
	auto loader = loaderCtx->getTemplateLoader();
	if (not loader) {
		throw std::runtime_error{"cannot extend a template without specifying a template loader!"};
	}

	auto content = loader(name);
	auto base_context = std::make_unique<Context>(context.get());
	pegtl::parse<pegtl::if_must<grammar::grammar, pegtl::eof>, actions::action>(
		pegtl::memory_input{content, ""}, 
		base_context
	);
	
    context->addRenderToken([ctx=context.get(), base_context=std::move(base_context)]() -> Context::RenderOutput {
        return {std::move((*base_context)().rendered), true};
    });
}

}
}
