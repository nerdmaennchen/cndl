#pragma once

#include "context.h"
#include "../tao/pegtl.hpp"

#include <vector>
#include <map>
#include <string_view>

namespace qrqma {

namespace grammar {

struct for_control_statement;
struct for_identifier;
struct for_container_identifier;
struct for_content;

}

namespace actions {

namespace detail {
using SymbolList = std::vector<std::string>;
}

template<typename Rule> 
struct action;
namespace pegtl = tao::pegtl;

template <> struct action<grammar::for_control_statement> : pegtl::change_states<detail::SymbolList, ContextP, ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match(Input& in, ContextP& context)
    {
        char const* marker = in.current();
        auto container_ctx = std::make_unique<Context>(context.get());
        auto inner_context = std::make_unique<Context>(context.get());
        return pegtl::change_states<detail::SymbolList, ContextP, ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<3>{}, in, detail::SymbolList{}, container_ctx, inner_context, context, marker);
    }

    static void success(const std::string &in, detail::SymbolList &symbol_names, ContextP &container_ctx, ContextP &inner_context, ContextP &outer_context);

    template <typename Input> 
    static void success(const Input &in, detail::SymbolList &symbol_names, ContextP &container_ctx, ContextP &inner_ctx, ContextP &outer_ctx, char const* marker) {
        success(std::string{marker, std::size_t(in.current()-marker)}, symbol_names, container_ctx, inner_ctx, outer_ctx);
    }
};

template <> struct action<grammar::for_identifier> {
    static void apply(const std::string &in, detail::SymbolList &symbol_names, ContextP& inner_context);
    template <typename Input> 
    static void apply(const Input &in, detail::SymbolList &symbol_names, ContextP&, ContextP& inner_context) {
        apply(in.string(), symbol_names, inner_context);
    }
};
template <> struct action<grammar::for_container_identifier> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, detail::SymbolList&, ContextP& container_context, ContextP& outer_context) 
    {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, container_context, outer_context);
    }


    template <typename Input> 
    static void success(const Input &, ContextP&, ContextP&) {}
};


template <> struct action<grammar::for_content> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, detail::SymbolList&, ContextP&, ContextP& inner_context)
    {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, inner_context );
    }

    template <typename Input> 
    static void success(const Input &, ContextP&) { }
};

}
}