#pragma once

#include "context.h"


namespace qrqma {
namespace grammar {
struct text;
struct raw_text;
}

namespace actions {

template <> struct action<grammar::text> {
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        context->addRenderToken(Context::StaticText{in.string()});
    }
};
template <> struct action<grammar::raw_text> {
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        context->addRenderToken(Context::StaticText{in.string()});
    }
};

} // namespace actions
} // namespace qrqma