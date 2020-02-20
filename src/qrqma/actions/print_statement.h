#pragma once

#include "context.h"

namespace qrqma {
namespace grammar {
struct print_expression;
}

namespace actions {

template<typename Rule> 
struct action;

template <>
struct action<grammar::print_expression> {
    static void apply(ContextP &context);
    template <typename Input>
    static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

}
}