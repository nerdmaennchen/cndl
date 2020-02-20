#include "for_statement.h"
#include "../demangle.h"
#include "../overloaded.h"
#include "../finally.h"

#include <vector>
#include <string_view>
#include <stdexcept>

namespace qrqma {
namespace actions {

namespace {

Context::RenderOutput handleList(types::List& l, std::string &symName, Context* inner_context) {
    std::string ret;
    
    auto parent = inner_context->getParentContext();
    Context symbol_ctx{parent};
    inner_context->setParentContext(&symbol_ctx);
    // when leaving the loop promote the symbols from inner_contxt to the outer_context;
    internal::Finally f{[&]{
        inner_context->setParentContext(parent);

        auto const& symbols = inner_context->getSymbolTable();
        for (auto const& [k, v]: symbols) {
            parent->setSymbol(k, v);
        }
        // promote the loop variable as well
        parent->setSymbol(symName, symbol_ctx[symName]);
    }};

    symbol_ctx.setSymbol("loop.length", types::NonconstantExpression {
        [s=static_cast<types::Integer>(l.size())]{ 
            return s;
        }
    });

    for (types::Integer itCounter{}; itCounter < static_cast<types::Integer>(l.size()); ++itCounter) {
        symbol_ctx.setSymbol("loop.index0", types::NonconstantExpression{[c=itCounter]{  return c; }});
        symbol_ctx.setSymbol("loop.index",  types::NonconstantExpression{[c=itCounter+1]{  return c; }});
        symbol_ctx.setSymbol("loop.revindex0", types::NonconstantExpression{[c=l.size()-itCounter-1]{  return c; }});
        symbol_ctx.setSymbol("loop.revindex",  types::NonconstantExpression{[c=l.size()-itCounter]{  return c; }});

        symbol_ctx.setSymbol("loop.first",  types::NonconstantExpression{[v=itCounter == 0]{  return v; }});
        symbol_ctx.setSymbol("loop.last",  types::NonconstantExpression{[v=itCounter == static_cast<types::Integer>(l.size()-1)]{  return v; }});

        if (itCounter == 0) {
            symbol_ctx.setSymbol("loop.previtem", types::NonconstantExpression{});    
        } else {
            symbol_ctx.setSymbol("loop.previtem", types::NonconstantExpression{[val=l[itCounter-1]]{ return val; }});
        }
        if (itCounter == static_cast<types::Integer>(l.size()-1)) {
            symbol_ctx.setSymbol("loop.nextitem", types::NonconstantExpression{});    
        } else {
            symbol_ctx.setSymbol("loop.nextitem", types::NonconstantExpression{[val=l[itCounter+1]]{ return val; }});
        }

        symbol_ctx.setSymbol(symName, types::NonconstantExpression{[val=l[itCounter]]{ 
            return val;
        }});

        auto ro = (*inner_context)(false);

        ret += ro.rendered;
        if (ro.stop_rendering_flag) {
            return {ret, true};
        }
    }

    return {ret, false};
}

template<typename MapT>
Context::RenderOutput handleMap(MapT& m, detail::SymbolList &symbol_names, Context* inner_context) {
    std::string ret;
    
    auto parent = inner_context->getParentContext();
    Context symbol_ctx{parent};
    inner_context->setParentContext(&symbol_ctx);
    // when leaving the loop promote the symbols from inner_contxt to the outer_context;
    internal::Finally f{[&]{
        inner_context->setParentContext(parent);

        auto const& symbols = inner_context->getSymbolTable();
        for (auto const& [k, v]: symbols) {
            parent->setSymbol(k, v);
        }
        // promote the loop variable as well
        for (auto const& symName : symbol_names) {
            parent->setSymbol(symName, symbol_ctx[symName]);
        }
    }};

    symbol_ctx.setSymbol("loop.length", types::NonconstantExpression {
        [s=static_cast<types::Integer>(m.size())]{ 
            return s;
        }
    });

    types::Integer itCounter {};
    auto prev = std::begin(m);
    for (auto it=std::begin(m); it!=std::end(m); ++it) {
        symbol_ctx.setSymbol("loop.index0", types::NonconstantExpression{[c=itCounter]{  return c; }});
        symbol_ctx.setSymbol("loop.index",  types::NonconstantExpression{[c=itCounter+1]{  return c; }});
        symbol_ctx.setSymbol("loop.revindex0", types::NonconstantExpression{[c=m.size()-itCounter-1]{  return c; }});
        symbol_ctx.setSymbol("loop.revindex",  types::NonconstantExpression{[c=m.size()-itCounter]{  return c; }});

        symbol_ctx.setSymbol("loop.first",  types::NonconstantExpression{[v=itCounter == 0]{  return v; }});
        symbol_ctx.setSymbol("loop.last",  types::NonconstantExpression{[v=itCounter == static_cast<types::Integer>(m.size()-1)]{  return v; }});

        auto next = std::next(it);
        if (next == std::end(m)) {
            symbol_ctx.setSymbol("loop.nextitem", types::NonconstantExpression{});    
        } else {
            symbol_ctx.setSymbol("loop.nextitem", types::NonconstantExpression{[val=next->first]{return val;}});
        }

        if (it == prev) {
            symbol_ctx.setSymbol("loop.previtem", types::NonconstantExpression{});    
        } else {
            symbol_ctx.setSymbol("loop.previtem", types::NonconstantExpression{[val=prev->first]{return val;}});
        }

        symbol_ctx.setSymbol(symbol_names[0], types::NonconstantExpression{[val=it->first]{return val;}});
        if (symbol_names.size() == 2) {
            symbol_ctx.setSymbol(symbol_names[1], types::NonconstantExpression{[val=it->second]{return val;}});
        }

        auto ro = (*inner_context)(false);

        ret += ro.rendered;
        if (ro.stop_rendering_flag) {
            return {ret, true};
        }

        ++itCounter;
        prev = it;
    }

    return {ret, false};
}

}

void action<grammar::for_control_statement>::success(const std::string &in, detail::SymbolList &symbol_names, ContextP &container_ctx, ContextP &inner_context, ContextP &outer_context) {
    outer_context->addRenderToken([in=std::move(in), symbol_names=std::move(symbol_names), container_expr=container_ctx->popExpression(), container_ctx=std::move(container_ctx), inner_context=std::move(inner_context), outer_context=outer_context.get()]() mutable -> Context::RenderOutput
    {
        auto iterable = std::visit([](auto& expr){ return expr.eval(); }, container_expr);
        if (iterable.type() == typeid(symbol::List)) {
            if (symbol_names.size() != 1) {
                throw std::runtime_error("cannot broadcast list items into " + std::to_string(symbol_names.size()) + " identifiers: \"" + in + "\"");
            }
            auto& l = std::any_cast<types::List&>(iterable);
            return handleList(l, symbol_names[0], inner_context.get());
        } else if (iterable.type() == typeid(symbol::Map)) {
            if (symbol_names.size() > 2) {
                throw std::runtime_error("cannot broadcast list items into more than two identifiers: \"" + in + "\"");
            }
            auto& m = std::any_cast<types::Map&>(iterable);
            return handleMap(m, symbol_names, inner_context.get());
        } else if (iterable.type() == typeid(symbol::UnorderedMap)) {
            if (symbol_names.size() > 2) {
                throw std::runtime_error("cannot broadcast list items into more than two identifiers: \"" + in + "\"");
            }
            auto& m = std::any_cast<types::UnorderedMap&>(iterable);
            return handleMap(m, symbol_names, inner_context.get());
        } else if (iterable.type() == typeid(symbol::MultiMap)) {
            if (symbol_names.size() > 2) {
                throw std::runtime_error("cannot broadcast list items into more than two identifiers: \"" + in + "\"");
            }
            auto& m = std::any_cast<types::MultiMap&>(iterable);
            return handleMap(m, symbol_names, inner_context.get());
        } else if (iterable.type() == typeid(symbol::UnorderedMultiMap)) {
            if (symbol_names.size() > 2) {
                throw std::runtime_error("cannot broadcast list items into more than two identifiers: \"" + in + "\"");
            }
            auto& m = std::any_cast<types::UnorderedMultiMap&>(iterable);
            return handleMap(m, symbol_names, inner_context.get());
        }
        throw std::runtime_error{"cannot loop over something of type: \"" + internal::demangle(iterable.type()) + "\""};
        return {"", false};
    });
}

void action<grammar::for_identifier>::apply(const std::string &name, detail::SymbolList &symbol_names, ContextP&) {
    symbol_names.emplace_back(name);
}

}
}