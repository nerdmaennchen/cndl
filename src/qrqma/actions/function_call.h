#pragma once

#include "context.h"
#include "../tao/pegtl.hpp"

namespace qrqma {
namespace grammar {

namespace ops {

struct call_identifier;
struct arg_list;
struct call;

}
}

namespace actions {

template<typename Rule> 
struct action;

namespace pegtl = tao::pegtl;

template <> struct action<grammar::ops::call> : pegtl::change_states<ContextP, ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& outer_context)
    {
        auto argsC = std::make_unique<Context>(outer_context.get());
        char const* marker = in.current();
        return pegtl::change_states<ContextP, ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<2>{}, in, outer_context, argsC, marker);
    }

    static void success(std::string const& in, ContextP& outer_context, ContextP& argsC);
    template <typename Input> 
    static void success(const Input &in, ContextP& outer_context, ContextP& argsC, char const* marker) {
        success(std::string{marker, std::size_t(in.current()-marker)}, outer_context, argsC);
    }
};

template <> struct action<grammar::ops::call_identifier> {
    static void apply(const std::string &in, ContextP& outer_context, ContextP& argsC);
    template <typename Input>
    static void apply(const Input &in, ContextP& outer_context, ContextP& argsC) {
        apply(in.string(), outer_context, argsC);
    }
};

template <> struct action<grammar::ops::arg_list> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& outer_context, ContextP& argsC) {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, argsC, outer_context);
    }

    template <typename Input> 
    static void success(const Input &,ContextP&, ContextP&) {}
};

} // namespace actions
} // namespace qrqma
