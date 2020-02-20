#include "index_operator.h"
#include "convert.h"

#include "../demangle.h"
#include "types.h"
#include <stdexcept>

namespace qrqma {
namespace actions {

void action<grammar::ops::index>::apply(const std::string &in, ContextP &context) {
    // the rightmost expression is the index expression; the previous one is the target
    auto idxExpr = context->popExpression();
    auto targetExpr = context->popExpression();
    
    bool constantExpr = std::holds_alternative<types::ConstantExpression>(idxExpr) and std::holds_alternative<types::ConstantExpression>(idxExpr);

    if (constantExpr) {
        auto t = std::get<types::ConstantExpression>(targetExpr).eval();
        
        if (t.type() == typeid(types::List)) {
            auto idx = convert<types::Integer>(std::get<types::ConstantExpression>(idxExpr).eval());
            auto val = std::any_cast<types::List const&>(t).at(idx);
            context->pushExpression(types::ConstantExpression{[val=std::move(val)] {
                return val;
            }});
        } else if (t.type() == typeid(types::Map)) {
            auto idx = convert<types::String>(std::get<types::ConstantExpression>(idxExpr).eval());
            auto val = std::any_cast<types::Map const&>(t).at(idx);
            context->pushExpression(types::ConstantExpression{[val=std::move(val)] {
                return val;
            }});
        } else {
            throw std::runtime_error("in line: \n" + in + "\ncannot utilize symbol of type \"" + internal::demangle(t.type()) + "\" as list or map");
        }
    } else {
        context->pushExpression(types::NonconstantExpression{[idxExpr=std::move(idxExpr), targetExpr=std::move(targetExpr), in=std::move(in)]{
            auto t = std::visit([](auto const& targetExpr) { return targetExpr.eval(); }, targetExpr);
            if (t.type() == typeid(types::List)) {
                auto idx = convert<types::Integer>(std::visit([](auto const& idxExpr) { return idxExpr.eval(); }, idxExpr));
                return std::any_cast<types::List const&>(t).at(idx);
            } else if (t.type() == typeid(types::Map)) {
                auto idx = convert<types::String>(std::visit([](auto const& idxExpr) { return idxExpr.eval(); }, idxExpr));
                return std::any_cast<types::Map const&>(t).at(idx);
            } else {
                throw std::runtime_error("in line: \n" + in + "\ncannot utilize symbol of type \"" + internal::demangle(t.type()) + "\" as list or map");
            }
        }});
    }
}

} // namespace actions
} // namespace qrqma
