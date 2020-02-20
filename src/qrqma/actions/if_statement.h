#pragma once

#include "context.h"
#include "../tao/pegtl.hpp"


namespace qrqma {
namespace grammar {
struct if_control_statement;
struct elif_control_statement;
struct conditional_statement;
struct if_content;
struct else_control_statement;
}

namespace actions {

template<typename Rule> 
struct action;
namespace pegtl = tao::pegtl;

template <> struct action<grammar::if_control_statement> : pegtl::change_states<ContextP, ContextP, ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& oldC)
    {
        auto ifC = std::make_unique<Context>(oldC.get());
        auto elseC = std::make_unique<Context>(oldC.get());
        return pegtl::change_states<ContextP, ContextP, ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<3>{}, in, oldC, ifC, elseC );
    }

    static void success(ContextP& outerC, ContextP& ifC, ContextP& elseC);
    template <typename Input> 
    static void success(const Input &,ContextP& outerC, ContextP& ifC, ContextP& elseC) {
        success(outerC, ifC, elseC);
    }
};
template <> struct action<grammar::elif_control_statement> : action<grammar::if_control_statement> {};

template <> struct action<grammar::conditional_statement> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& outerC, ContextP&, ContextP&)
    {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, outerC );
    }

    template <typename Input> 
    static void success(const Input &, ContextP&) { }
};

template <> struct action<grammar::if_content> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP&, ContextP& ifC, ContextP&)
    {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, ifC );
    }

    template <typename Input> 
    static void success(const Input &, ContextP&) { }
};

template <> struct action<grammar::else_control_statement> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP&, ContextP&, ContextP& elseC)
    {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, elseC );
    }

    template <typename Input> 
    static void success(const Input &, ContextP&) { }
};


}
}