#pragma once

#include "context.h"

#include "../tao/pegtl.hpp"

namespace qrqma {
namespace grammar {

struct string_literal;
struct integer;
struct float_num;
struct bool_true;
struct bool_false;
struct identifier;
struct atom_list;
struct atom_map;

}


namespace actions {
namespace pegtl = tao::pegtl;

template<typename Rule> 
struct action;

template <> 
struct action<grammar::string_literal> {
    static void apply(const std::string &in, ContextP &context);
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        apply(in.string(), context);
    }
};

template <> struct action<grammar::integer> {
    static void apply(const std::string &in, ContextP &context);
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        apply(in.string(), context);
    }
};

template <> struct action<grammar::float_num> {
    static void apply(const std::string &in, ContextP &context);
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        apply(in.string(), context);
    }
};

template <> struct action<grammar::bool_true> {
    static void apply(const std::string &in, ContextP &context);
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        apply(in.string(), context);
    }
};
template <> struct action<grammar::bool_false> {
    static void apply(const std::string &in, ContextP &context);
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        apply(in.string(), context);
    }
};

template <>
struct action<grammar::identifier> {
    static void apply(const std::string &in, ContextP &context);
    template <typename Input>
    static void apply(const Input &in, ContextP &context) {
        apply(in.string(), context);
    }
};

template <> struct action<grammar::atom_list> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& context)
    {
        auto childContext = std::make_unique<Context>(context.get());
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, childContext, context );
    }

    static void success(ContextP& inner_context, ContextP& outer_context);
    template <typename Input> 
    static void success(const Input &, ContextP& inner_context, ContextP& outer_context) {
        success(inner_context, outer_context);
    }
};

template <> struct action<grammar::atom_map> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& context)
    {
        auto childContext = std::make_unique<Context>(context.get());
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, childContext, context );
    }


    static void success(ContextP& inner_context, ContextP& outer_context);
    template <typename Input> 
    static void success(const Input &, ContextP& inner_context, ContextP& outer_context) {
        success(inner_context, outer_context);
    }
};

} // namespace actions
} // namespace qrqma