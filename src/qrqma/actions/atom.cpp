#include "atom.h"
#include "convert.h"

#include "../overloaded.h"
#include "types.h"

#include <string>
#include <cstdint>
#include <sstream>
#include <utility>

#include <algorithm>

namespace qrqma {
namespace actions {

void action<grammar::string_literal>::apply(const std::string &in, ContextP &context) {
    std::string s{in.data() + 1, in.size() - 2}; // the string without the quotation marks;
    context->pushExpression(types::ConstantExpression{ [s = std::move(s)]{ return s; }});
}

void action<grammar::integer>::apply(const std::string &in, ContextP &context) {
    types::Integer num{};
    std::stringstream{in} >> num;
    context->pushExpression(types::ConstantExpression{[num = std::move(num)]{ return num; }});
}

void action<grammar::float_num>::apply(const std::string &in, ContextP &context) {
    types::Float num{};
    std::stringstream{in} >> num;
    context->pushExpression(types::ConstantExpression{[num = std::move(num)]{ return num; }});
}

void action<grammar::bool_true>::apply(const std::string&, ContextP &context) {
    context->pushExpression(types::ConstantExpression{[]{ return true; }});
}
void action<grammar::bool_false>::apply(const std::string&, ContextP &context) {
    context->pushExpression(types::ConstantExpression{[]{ return false; }});
}

void action<grammar::identifier>::apply(const std::string &in, ContextP &context) {
    auto value = (*context)[in];
    if (value) {
        context->pushExpression(std::visit(detail::overloaded{
            [](types::ConstantExpression& ce) -> types::Expression {
                return types::ConstantExpression{[val=ce.eval()] { return val; }};
            },
            [value](types::NonconstantExpression&) -> types::Expression {
                return types::NonconstantExpression{[value] { // capture "value" (a shared_ptr) here
                    return std::visit([](auto&& expr) { return expr.eval(); }, *value);
                }};
            }
        }, *value));
    } else {
        // delayed evaluation
        context->pushExpression(types::NonconstantExpression{[c=context.get(), in]() {
            auto v = (*c)[in];
            if (not v) {
                return std::any{};
            }
            return std::visit([](auto&& expr) { return expr.eval(); }, *v);
        }});
    }
}

void action<grammar::atom_list>::success(ContextP& inner_context, ContextP& outer_context) {
    auto items = inner_context->popAllExpressions();
    bool allConstant = std::all_of(begin(items), end(items), [](auto const& e) { return std::holds_alternative<types::ConstantExpression>(e); });

    if (allConstant) {
        types::List l;
        for (auto& e : items) {
            l.emplace_back(std::visit([](auto const& expr) -> types::Value {
                return expr.eval();
            }, e));
        }
        outer_context->pushExpression(types::ConstantExpression{[l = std::move(l)] {
            return l;
        }});
    } else {
        outer_context->pushExpression(types::NonconstantExpression{[items = std::move(items)]{
            types::List l;
            for (auto& e : items) {
                l.emplace_back(std::visit([](auto const& expr) -> types::Value {
                    return expr.eval();
                }, e));
            }
            return l;
        }});
    }
}

void action<grammar::atom_map>::success(ContextP& inner_context, ContextP& outer_context) {
    auto items = inner_context->popAllExpressions();
    bool allConstant = std::all_of(begin(items), end(items), [](auto const& e) { return std::holds_alternative<types::ConstantExpression>(e); });

    if (allConstant) {
        types::Map m;
        for (auto k_it=items.begin(); k_it != items.end(); std::advance(k_it, 2)) {
            auto v_it = std::next(k_it);
            std::string key = std::visit([&](auto const& expr) -> std::string {
                auto key = expr.eval();
                return convert<std::string>(key);
            }, *k_it);
            std::any val = std::visit([](auto const& expr) -> std::any {
                return expr.eval();
            }, *v_it);
            m.emplace(std::move(key), std::move(val));
        }
        outer_context->pushExpression(types::ConstantExpression{[m = std::move(m)] {
            return m;
        }});
    } else {
        outer_context->pushExpression(types::NonconstantExpression{[items=std::move(items), outer_context=outer_context.get()] {
            types::Map m;
            for (auto k_it=items.begin(); k_it != items.end(); std::advance(k_it, 2)) {
                auto v_it = std::next(k_it);
                std::string key = std::visit([&](auto const& expr) -> std::string {
                    auto key = expr.eval();
                    return convert<std::string>(key);
                }, *k_it);
                std::any val = std::visit([](auto const& expr) -> std::any {
                    return expr.eval();
                }, *v_it);
                m.emplace(std::move(key), std::move(val));
            }
            return m;
        }});
    }
}

} // namespace actions
} // namespace qrqma