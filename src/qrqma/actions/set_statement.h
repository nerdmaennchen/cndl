#pragma once

#include "context.h"

#include "../tao/pegtl.hpp"

namespace qrqma {
namespace grammar {
struct set_control_statement;
struct set_identifier;
struct set_expression;
}

namespace actions {

namespace pegtl = tao::pegtl;

template<typename Rule> 
struct action;

template <> struct action<grammar::set_control_statement> : pegtl::change_states<std::string, ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& context)
    {
        return pegtl::change_states<std::string, ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<2>{}, in, std::string{}, context );
    }

    static void success(std::string &symbol_name, ContextP& context);
    template <typename Input> 
    static void success(const Input &, std::string &symbol_name, ContextP& context) {
        success(symbol_name, context);
    }
};

template <> struct action<grammar::set_identifier> {
    template <typename Input> 
    static void apply(const Input &in, std::string &symbol_name, ContextP&) {
        symbol_name = in.string();
    }
};

template <> struct action<grammar::set_expression> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, std::string&, ContextP& context)
    {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, context );
    }

    template <typename Input> 
    static void success(const Input &, ContextP&) { }
};

}
}