#pragma once

#include "context.h"
#include "types.h"

namespace qrqma {

namespace grammar {
struct expression;

namespace ops {
struct unary_minus;
struct unary_not;
struct star;
struct fslash;
struct percent;
struct plus;
struct minus;
struct cmp_lt;
struct cmp_leq;
struct cmp_gt;
struct cmp_geq;
struct cmp_eq;
struct cmp_neq;
struct op_and;
struct op_or;
}
}

namespace actions {
template <typename Rule> 
struct action;

template <> struct action<grammar::ops::unary_minus> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::unary_not> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::star> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::fslash> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::percent> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::plus> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::minus> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::cmp_lt> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::cmp_leq> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::cmp_gt> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::cmp_geq> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::cmp_eq> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::cmp_neq> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::op_and> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

template <> struct action<grammar::ops::op_or> {
    static void apply(ContextP &context);
    template <typename Input> static void apply(const Input &, ContextP &context) {
        apply(context);
    }
};

} // namespace actions
} // namespace qrqma