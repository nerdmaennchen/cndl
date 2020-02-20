#pragma once

#include "context.h"
#include "../tao/pegtl.hpp"

namespace qrqma {

namespace grammar {
struct block_control_statement;
struct block_identifier;
struct block_content;   
struct block_scoped;   
}

namespace actions {

namespace pegtl = tao::pegtl;

template<typename Rule> 
struct action;

template <> struct action<grammar::block_control_statement> : pegtl::change_states<std::string, ContextP, bool> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, ContextP& oldC) {
        auto childContext = std::make_unique<Context>(oldC.get());
        bool scoped {false};
        return pegtl::change_states<std::string, ContextP, bool>::match< Rule, A, M, Action, Control >(std::make_index_sequence<3>{}, in, std::string{}, childContext, scoped, oldC );
    }

    static void success(std::string& block_name, ContextP& inner_ctx, bool scoped, ContextP& outer_ctx);
    template <typename Input>
    static void success(const Input &, std::string& block_name, ContextP& inner_ctx, bool scoped, ContextP& outer_ctx) {
        success(block_name, inner_ctx, scoped, outer_ctx);
    }
};

template <> struct action<grammar::block_identifier> {
    template <typename Input> 
    static void apply(const Input &in, std::string &block_name, ContextP&, bool&) {
        block_name = in.string();
    }
};

template <> struct action<grammar::block_scoped> {
    template <typename Input> 
    static void apply(const Input &, std::string &, ContextP&, bool& scoped) {
        scoped = true;
    }
};

template <> struct action<grammar::block_content> : pegtl::change_states<ContextP> {
    template< typename Rule, pegtl::apply_mode A, pegtl::rewind_mode M, template< typename... > class Action, template< typename... > class Control, typename Input>
    [[nodiscard]] static bool match( Input& in, std::string&, ContextP& ctx, bool&) {
        return pegtl::change_states<ContextP>::match< Rule, A, M, Action, Control >(std::make_index_sequence<1>{}, in, ctx );
    }

    template <typename Input> 
    static void success(const Input &, ContextP&) { }
};

}
}