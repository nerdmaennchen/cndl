#include "template.h"

#include "actions/actions.h"

#include "actions/context.h"
#include "actions/types.h"
#include "grammar/grammar.h"

#include <map>
#include <string>

#include "finally.h"

namespace qrqma {

struct Template::Pimpl {
    actions::Context symbol_context;
    actions::Context render_context;

    Pimpl(symbol::SymbolTable symbols, TemplateLoader loader, symbol::BlockTable blocks)
    : symbol_context{{}, std::move(blocks)}
    , render_context{&symbol_context} 
    {
        for (auto& [k, v] : symbols) {
            symbol_context.setSymbol(k, actions::types::ConstantExpression{[v=std::move(v)] { return v; }});
        }
        symbol_context.setTemplateLoader(std::move(loader));
    }
};

namespace pegtl = tao::pegtl;
Template::Template(std::string_view input, symbol::SymbolTable symbols, TemplateLoader loader, symbol::BlockTable blocks)
    : pimpl{std::make_unique<Pimpl>(std::move(symbols), std::move(loader), std::move(blocks))} 
{
    pegtl::parse<pegtl::if_must<grammar::grammar, pegtl::eof>, actions::action>(
        pegtl::memory_input{input, ""}, 
        pimpl->render_context
    );
}

Template::Template(Template&& rhs) {
    std::swap(pimpl, rhs.pimpl);
}

Template& Template::operator=(Template&& rhs) {
    std::swap(pimpl, rhs.pimpl);
    return *this;
}
    

Template::~Template() {}

std::string Template::operator()(symbol::SymbolTable symbols) const {
    actions::Context rootC{};
    for (auto& [k, v] : symbols) {
        rootC.setSymbol(k, actions::types::ConstantExpression{ [v=std::move(v)] { return v; } });
    }
    pimpl->symbol_context.setParentContext(&rootC);
    internal::Finally reset {[this]{
        pimpl->symbol_context.setParentContext(nullptr);
    }};
    return std::move(pimpl->render_context(false).rendered);
}

} // namespace qrqma