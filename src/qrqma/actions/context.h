#pragma once

#include <map>
#include <variant>
#include <vector>
#include <typeinfo>
#include <optional>
#include <memory>

#include "types.h"
#include "../loader.h"
#include "../symbol.h"
#include "../unique_function.h"

namespace qrqma {
namespace actions {

struct Context {
    struct StopToken {};
    using StaticText   = symbol::StaticText;
    using Renderable   = symbol::Renderable;
    using ContextBlock = symbol::ContextBlock;
    using RenderToken = std::variant<StaticText, Renderable, StopToken>;

    using RenderOutput = symbol::RenderOutput;

    using Expression      = types::Expression;
    using ExpressionPtr   = types::ExpressionPtr;
    using ExpressionTable = types::ExpressionTable;

    using Block       = symbol::Block;
    using BlockTable  = symbol::BlockTable;

    Context() = default;
    Context(ExpressionTable expressions, BlockTable blocks);
    Context(Context* parent);

    void setTemplateLoader(TemplateLoader loader) {
        templateLoader = std::move(loader);
    }
    TemplateLoader getTemplateLoader() const {
        return *templateLoader;
    }

    void addRenderToken(RenderToken t);

    void pushExpression(Expression f);
    Expression popExpression();
    std::vector<Expression> popAllExpressions();

    auto expressionStackSize() const {
        return expression_stack.size();
    }

    ExpressionPtr operator[](std::string const &name);
    ExpressionTable const& getSymbolTable() const;
    
    void setSymbol(std::string const& name, Expression symbol);
    void setSymbol(std::string const& name, ExpressionPtr symbol);

    Block const& getBlock(std::string const &name) const;
    void setBlock(std::string const& name, Block block);
    BlockTable popBlockTable();

    RenderOutput operator()(bool promoteSymbols=true);

    Context* getParentContext() {
        return parent;
    }
    void setParentContext(Context* p) {
        parent = p;
    }

private:
    std::vector<RenderToken> tokens;

    ExpressionTable symbols;
    BlockTable blocks;

    std::vector<Expression> expression_stack;

    std::optional<TemplateLoader> templateLoader {nullptr};

    Context* parent{nullptr};
};

using ContextP = std::unique_ptr<Context>;

}
}
