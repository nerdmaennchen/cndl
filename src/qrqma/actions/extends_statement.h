#pragma once

#include "context.h"
#include <vector>
#include <string_view>

namespace qrqma {
namespace grammar {
struct extends_control_statement;
}

namespace actions {

template <typename Rule> 
struct action;

template <> struct action<grammar::extends_control_statement> {
    template <typename Input> 
    static void apply(const Input &, ContextP& context) {
        apply(context);
    }
    static void apply(ContextP& context);
};

}
}