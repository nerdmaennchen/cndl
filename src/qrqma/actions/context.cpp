#include "context.h"

#include "../overloaded.h"

#include <numeric>

namespace qrqma {
namespace actions {

Context::ExpressionPtr Context::operator[](std::string const &name) {
    Context *context{this};
    while (context) {
        auto it = context->symbols.find(name);
        if (it != context->symbols.end()) {
            return it->second;
        }
        context = context->parent;
    };
    return nullptr;
}
Context::ExpressionTable const& Context::getSymbolTable() const { return symbols; }

void Context::setSymbol(std::string const& name, Expression symbol) {
    symbols.insert_or_assign(name, std::make_shared<Expression>(std::move(symbol)));
}
void Context::setSymbol(std::string const& name, ExpressionPtr symbol) {
    symbols.insert_or_assign(name, symbol);
}

Context::Block const& Context::getBlock(std::string const &name) const {
    Context const *context{this};
    Context::Block const* block {nullptr};
    while (context) {
        auto it = context->blocks.find(name);
        if (it != context->blocks.end()) {
            block = &it->second;
        }
        context = context->parent;
    }
    if (block) {
        return *block;
    }
    throw std::runtime_error("no block with name \"" + name + "\" provided!");
}
void Context::setBlock(std::string const& name, Block block) {
    blocks.emplace(name, std::move(block));
}
Context::BlockTable Context::popBlockTable() { 
    BlockTable table;
    std::swap(table, blocks);
    return table; 
}

Context::RenderOutput Context::operator()(bool promoteSymbols) {
    RenderOutput ro;

    symbols.clear();
    for (auto const &t : tokens) {
        auto ro_sub = std::visit(detail::overloaded{
            [](StaticText const &t) -> RenderOutput { return {t, false}; },
            [](Renderable const &c) -> RenderOutput { return c(); },
            [](auto const&) -> RenderOutput { return {"", true}; }},
        t);
        ro.rendered += ro_sub.rendered;
        ro.stop_rendering_flag = ro_sub.stop_rendering_flag;
        if (ro.stop_rendering_flag) {
            break;
        }
    }
    if (promoteSymbols and parent) {
        // promote the symbols to the parent context
        for (auto const&[k, v] : symbols) {
            parent->setSymbol(k, v);
        }
    }

    return ro;
}

void Context::addRenderToken(RenderToken t) { tokens.emplace_back(std::move(t)); }


void Context::pushExpression(Expression f) {
    expression_stack.emplace_back(std::move(f));
}

Context::Expression Context::popExpression() {
    auto e = std::move(expression_stack.back());
    expression_stack.pop_back();
    return e;
}

std::vector<Context::Expression> Context::popAllExpressions() {
    std::vector<Context::Expression> newStack;
    std::swap(newStack, expression_stack);
    return newStack;
}

Context::Context(ExpressionTable in_symbols, BlockTable in_blocks)
    : symbols{std::move(in_symbols)} 
    , blocks{std::move(in_blocks)}
{}

Context::Context(Context* c_parent)
    : parent{c_parent} {}

}
}
