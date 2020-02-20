#pragma once

#include "../symbol.h"
#include "../unique_function.h"

#include <typeinfo>
#include <cstdint>
#include <string>
#include <memory>

namespace qrqma {
namespace actions {
namespace types {

struct Undefined {};

using Integer = std::int64_t;
using Float = double;
using Bool = bool;
using String = std::string;

using Value = symbol::Symbol;

struct ExpressionBase {
    using FuncType = qrqma::unique_func<Value()>;

    ExpressionBase()
        : ExpressionBase{[]{return std::any{}; }} {}
    ExpressionBase(FuncType func)
        : mEval{std::move(func)} {}

    ExpressionBase(ExpressionBase&& other) noexcept = default;
    ExpressionBase& operator=(ExpressionBase&& other) noexcept = default;
    
    auto eval() const { return mEval(); }

private:
    FuncType mEval;
};

struct NonconstantExpression : ExpressionBase {
    using ExpressionBase::ExpressionBase;
    using ExpressionBase::operator=;

    NonconstantExpression(ExpressionBase&& other) noexcept : ExpressionBase{std::move(other)} {}
    NonconstantExpression& operator=(ExpressionBase&& other) noexcept {
        ExpressionBase::operator=(std::move(other));
        return *this;
    };
};
struct ConstantExpression : ExpressionBase {
    using ExpressionBase::ExpressionBase;
    using ExpressionBase::operator=;

    ConstantExpression(ExpressionBase&& other) noexcept : ExpressionBase{std::move(other)} {};
    ConstantExpression& operator=(ExpressionBase&& other) noexcept {
        ExpressionBase::operator=(std::move(other));
        return *this;
    };
};

using Expression = std::variant<NonconstantExpression, ConstantExpression>;

NonconstantExpression asNonconstantE(Expression&& e);

using ExpressionPtr = std::shared_ptr<Expression>;
using ExpressionTable = std::map<std::string, ExpressionPtr>;

using List = symbol::List;
using Map = symbol::Map;
using MultiMap = symbol::MultiMap;
using UnorderedMap = symbol::UnorderedMap;
using UnorderedMultiMap = symbol::UnorderedMultiMap;

}
}
}