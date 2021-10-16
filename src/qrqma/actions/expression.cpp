#include "expression.h"
#include "convert.h"
#include "../demangle.h"
#include "../overloaded.h"
#include "../unique_function.h"

#include <stdexcept>

namespace qrqma {
namespace actions {
namespace detail {

template<typename... Types>
struct TypeContainer;

template<typename... Types>
struct PrefixHelper;

template<>
struct PrefixHelper<TypeContainer<>> {
    template<typename Functor>
    static auto apply(std::any const& val, std::string_view name, Functor&&) -> std::any {
        throw std::runtime_error{"cannot apply unary operator \"" + std::string{name} + "\" to type: " + internal::demangle(val.type())};
    }
};

template<typename Type, typename... Types>
struct PrefixHelper<TypeContainer<Type, Types...>> {
    template<typename Functor>
    static auto apply(std::any const& val, std::string_view name, Functor&& f) -> std::any {
        using T = std::remove_cv_t<std::remove_reference_t<Type>>;
        if (val.type() == typeid(T)) {
            return std::invoke(std::forward<Functor>(f), std::any_cast<T>(val));
        } else {
            return PrefixHelper<TypeContainer<Types...>>::apply(val, name, std::forward<Functor>(f));
        }
    }
};

template<typename... Types>
struct InfixHelper;

template<typename... Types>
struct InfixInnerHelper;

template<>
struct InfixInnerHelper<TypeContainer<>> {
    template<typename LT, typename Functor>
    static auto apply(LT&&, std::any const& rhs, std::string_view name, Functor&&) -> std::any {
        throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(LT)) + " and " + internal::demangle(rhs.type())};
    }
};

template<typename RT, typename... RTypes>
struct InfixInnerHelper<TypeContainer<RT, RTypes...>> {
    template<typename LT, typename Functor>
    static auto apply(LT&& lhs, std::any const& rhs, std::string_view name, Functor&& f) -> std::any {
        using RT_ = std::remove_cv_t<std::remove_reference_t<RT>>;
        if (rhs.type() == typeid(RT_)) {
            return std::invoke(std::forward<Functor>(f), std::forward<LT>(lhs), std::any_cast<RT>(rhs));
        } else {
            return InfixInnerHelper<TypeContainer<RTypes...>>::apply(lhs, rhs, name, std::forward<Functor>(f));
        }
    }
};

template<typename... RTypes>
struct InfixHelper<TypeContainer<>, TypeContainer<RTypes...>> {
    template<typename Functor>
    static auto apply(std::any const& lhs, std::any const& rhs, std::string_view name, Functor&&) -> std::any {
        throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(lhs.type()) + " and " + internal::demangle(rhs.type())};
    }
};

template<typename LT, typename... LTypes, typename... RTypes>
struct InfixHelper<TypeContainer<LT, LTypes...>, TypeContainer<RTypes...>> {
    template<typename Functor>
    static auto apply(std::any const& lhs, std::any const& rhs, std::string_view name, Functor&& f) -> std::any {
        using LT_ = std::remove_cv_t<std::remove_reference_t<LT>>;
        if (lhs.type() == typeid(LT_)) {
            return InfixInnerHelper<TypeContainer<RTypes...>>::apply(std::any_cast<LT>(lhs), rhs, name, std::forward<Functor>(f));
        } else {
            return InfixHelper<TypeContainer<LTypes...>, TypeContainer<RTypes...>>::apply(lhs, rhs, name, std::forward<Functor>(f));
        }
    }
};

template<typename Helper, typename Op>
void buildInfixOp(ContextP &context, std::string_view name, Op op) {
    auto rhs = context->popExpression();
    context->pushExpression(std::visit(detail::overloaded {
        [&](types::ConstantExpression& r) -> types::Expression {
            auto lhs = context->popExpression();
            auto val_r = r.eval();
            return std::visit(detail::overloaded {
                [&](types::ConstantExpression& l) -> types::Expression {
                    auto val_l = l.eval();
                    auto res = Helper::apply(val_l, val_r, name, op);
                    return types::ConstantExpression{[res=std::move(res)]{ return res; }};
                },
                [&](types::NonconstantExpression& l) -> types::Expression {
                    return types::NonconstantExpression{[val_r=std::move(val_r), l=std::move(l), name, op]{
                        auto val_l = l.eval();
                        return Helper::apply(val_l, val_r, name, op);
                    }};
                }
            }, lhs);
        },
        [&](types::NonconstantExpression& r) -> types::Expression {
            auto lhs = context->popExpression();
            return std::visit(detail::overloaded {
                [&](types::ConstantExpression& l) -> types::Expression {
                    auto val_l = l.eval();
                    return types::NonconstantExpression{[val_l=std::move(val_l), r=std::move(r), name, op]{
                            auto val_r = r.eval();
                            return Helper::apply(val_l, val_r, name, op);
                        }};
                },
                [&](types::NonconstantExpression& l) -> types::Expression {
                    return types::NonconstantExpression{[r=std::move(r), l=std::move(l), name, op]{
                        auto val_l = l.eval();
                        auto val_r = r.eval();
                        auto res = Helper::apply(val_l, val_r, name, op);
                        return res;
                    }};
                }
            }, lhs);
        }
    }, rhs));
}

}

template<typename... Types>
using PrefixHelper = detail::PrefixHelper<detail::TypeContainer<Types...>>;

template<typename... Types>
using InfixHelper = detail::InfixHelper<detail::TypeContainer<Types...>, detail::TypeContainer<Types...>>;

using namespace std::literals;


void action<grammar::ops::unary_minus>::apply(ContextP &context) {
    auto e = context->popExpression();
    auto op = [](auto v) { return -v; };
    using Helper = PrefixHelper<types::Integer, types::Float, types::Bool>;
    context->pushExpression(std::visit(detail::overloaded{
        [=](types::ConstantExpression& e) -> types::Expression {
            auto val = Helper::apply(e.eval(), "unary minus"sv, op);
            return types::ConstantExpression{[val=std::move(val)] { return val; }};
        },
        [=](types::NonconstantExpression& e) -> types::Expression {
            return types::NonconstantExpression{[e=std::move(e), op] {
                return Helper::apply(e.eval(), "unary minus"sv, op);
            }};
        }
    }, e));
}
void action<grammar::ops::unary_not>::apply(ContextP &context) {
    using namespace std::literals;
    auto e = context->popExpression();
    context->pushExpression(std::visit(detail::overloaded{
        [](types::ConstantExpression& e) -> types::Expression {
            return types::ConstantExpression{[val=convert<types::Bool>(e.eval())] { return not val; }};
        },
        [](types::NonconstantExpression& e) -> types::Expression {
            return types::NonconstantExpression{[e=std::move(e)] {
                return not convert<types::Bool>(e.eval());
            }};
        }
    }, e));
}

void action<grammar::ops::star>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto name = "operator mult"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [](types::String const& l, types::Integer r) { std::string res{}; while(r --> 0) { res+=l; } return res; },
        [](types::Integer r, types::String const& l) { std::string res{}; while(r --> 0) { res+=l; } return res; },
        [](types::Integer l, types::Integer r) { return l * r; },
        [](types::Integer l, types::Float r) { return l * r; },
        [](types::Integer l, types::Bool r) { return l * r; },
        [](types::Float l, types::Integer r) { return l * r; },
        [](types::Float l, types::Float r) { return l * r; },
        [](types::Float l, types::Bool r) { return l * r; },
        [](types::Bool l, types::Integer r) { return l * r; },
        [](types::Bool l, types::Float r) { return l * r; },
        [](types::Bool l, types::Bool r) { return l * r; },
        [name](auto&& l, auto&& r) -> int {
            throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(l)) + " and " + internal::demangle(typeid(r))};
        }
    });
}

void action<grammar::ops::fslash>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool>;
    detail::buildInfixOp<Helper>(context, "operator div"sv, detail::overloaded{
        [](auto l, auto r) { return l / r; }
    });
}

void action<grammar::ops::percent>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Bool>;
    detail::buildInfixOp<Helper>(context, "operator mod"sv, detail::overloaded{
        [](auto l, auto r) { return l % r; }
    });
}

void action<grammar::ops::plus>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l + r; };
    auto name = "operator plus"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [name](auto l, auto r) -> types::Bool {
            throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(l)) + " and " + internal::demangle(typeid(r))};
    }});
}

void action<grammar::ops::minus>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool>;
    detail::buildInfixOp<Helper>(context, "operator minus"sv, detail::overloaded{
        [](auto l, auto r) { return l - r; }
    });
}

void action<grammar::ops::cmp_lt>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l < r; };
    auto name = "operator cmp_lt"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [name](auto l, auto r) -> types::Bool {
            throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(l)) + " and " + internal::demangle(typeid(r))};
    }});
}

void action<grammar::ops::cmp_leq>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l <= r; };
    auto name = "operator cmp_leq"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [name](auto l, auto r) -> types::Bool {
            throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(l)) + " and " + internal::demangle(typeid(r))};
    }});
}

void action<grammar::ops::cmp_gt>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l > r; };
    auto name = "operator cmp_gt"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [name](auto l, auto r) -> types::Bool {
            throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(l)) + " and " + internal::demangle(typeid(r))};
    }});
}

void action<grammar::ops::cmp_geq>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l >= r; };
    auto name = "operator cmp_geq"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [name](auto l, auto r) -> types::Bool {
            throw std::runtime_error{"cannot apply binary operator \"" + std::string{name} + "\" to types: " + internal::demangle(typeid(l)) + " and " + internal::demangle(typeid(r))};
        }
    });
}

void action<grammar::ops::cmp_eq>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l == r; };
    auto name = "operator cmp_eq"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [](auto, auto) { return false; }
    });
}

void action<grammar::ops::cmp_neq>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto op = [](auto&& l, auto&& r) { return l != r; };
    auto name = "operator cmp_neq"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [op](types::String const& l, types::String const& r) { return op(l, r); },
        [op](types::Integer l, types::Integer r) { return op(l, r); },
        [op](types::Integer l, types::Float r) { return op(l, r); },
        [op](types::Integer l, types::Bool r) { return op(l, r); },
        [op](types::Float l, types::Integer r) { return op(l, r); },
        [op](types::Float l, types::Float r) { return op(l, r); },
        [op](types::Float l, types::Bool r) { return op(l, r); },
        [op](types::Bool l, types::Integer r) { return op(l, r); },
        [op](types::Bool l, types::Float r) { return op(l, r); },
        [op](types::Bool l, types::Bool r) { return op(l, r); },
        [](auto, auto) { return true; }
    });
}

void action<grammar::ops::op_and>::apply(ContextP &context) {
    using Helper = InfixHelper<types::Integer, types::Float, types::Bool, types::String>;
    auto name = "operator cmp_and"sv;
    detail::buildInfixOp<Helper>(context, name, detail::overloaded{
        [](auto const& l, auto const& r) { return static_cast<bool>(l) && static_cast<bool>(r); },
        [](std::string const& l, std::string const& r) { return not l.empty() && r.empty(); },
        [](auto const& l, std::string const& r) { return static_cast<bool>(l) && r.empty(); },
        [](std::string const& l, auto const& r) { return not l.empty() && static_cast<bool>(r); },
    });
}

void action<grammar::ops::op_or>::apply(ContextP &context) {
    auto rhs = context->popExpression();
    auto lhs = context->popExpression();
    context->pushExpression(std::visit(detail::overloaded {
        [&](types::ConstantExpression& l) -> types::Expression {
            auto val_l = l.eval();
            if (val_l.has_value() and convert<types::Bool>(val_l)) {
                return std::move(l);
            }
            return std::visit(detail::overloaded {
                [&](types::ConstantExpression& r) -> types::Expression {
                    return std::move(r);
                },
                [&](types::NonconstantExpression& r) -> types::Expression {
                    return types::NonconstantExpression{[r=std::move(r)] {
                        return r.eval();
                    }};
                }
            }, rhs);
        },
        [&](types::NonconstantExpression& l) -> types::Expression {
            return types::NonconstantExpression{[l=std::move(l), rhs=std::move(rhs)]{
                auto val_l = l.eval();
                if (val_l.has_value() and convert<types::Bool>(val_l)) {
                    return val_l;
                }
                return std::visit(detail::overloaded {
                    [&](auto& r) {
                        return r.eval();
                    }
                }, rhs);
            }};
        }
    }, lhs));
}

} // namespace actions
} // namespace qrqma