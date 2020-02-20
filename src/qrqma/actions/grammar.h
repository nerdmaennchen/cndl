#pragma once

#include "context.h"
#include "../tao/pegtl.hpp"


namespace qrqma {
namespace grammar {
struct grammar;
}

namespace actions {
template <typename Rule> 
struct action;

namespace pegtl = tao::pegtl;

template <> struct action<grammar::grammar> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, Context& outer_context)
    {
        auto inner_context = std::make_unique<Context>(&outer_context);
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, inner_context, outer_context);
    }

    static void success(ContextP& inner_context, Context& outer_context);
    template <typename Input> 
    static void success(const Input &, ContextP& inner_context, Context& outer_context) {
        success(inner_context, outer_context);
    }
};

} // namespace actions
} // namespace qrqma