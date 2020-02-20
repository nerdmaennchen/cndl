#pragma once
#include "../tao/pegtl.hpp"
#include "spaces.h"
#include "atom.h"

namespace qrqma {
namespace grammar {
namespace pegtl = tao::pegtl;

struct expression;

namespace ops {

template<int precedence>
struct lhs_term : lhs_term<precedence-1> {};
struct lhs_expression : padded<ops::lhs_term<3>> {};

template<int precedence>
struct infix_term : infix_term<precedence-1> {};

template <typename op_sep, int precedence>
struct infix_op : pegtl::seq<op_sep, lhs_expression, pegtl::opt<infix_term<precedence>>> {};

template <typename op_sep, int precedence>
struct prefix_unary_op : pegtl::seq<op_sep, spaces, lhs_term<precedence>> {};

template<typename rule>
struct try_match : pegtl::if_must<pegtl::at<rule>, rule> {};

struct call_identifier : identifier {};
struct arg_list : pegtl::opt<pegtl::list<expression, pegtl::one<','>>> {};
struct call  : try_match<pegtl::seq<call_identifier, spaces, pegtl::one<'('>, arg_list, pegtl::one<')'> > > {};
struct index : try_match<pegtl::seq<identifier, spaces, pegtl::one<'['>, expression, pegtl::one<']'> > > {};

struct unary_plus  : prefix_unary_op<pegtl::one<'+'>, 3> {};
struct unary_minus : prefix_unary_op<pegtl::one<'-'>, 3> {};
struct unary_not   : prefix_unary_op<pegtl::sor<pegtl::one<'!'>, pegtl::keyword<'n', 'o', 't'>>, 3> {};

struct star    : infix_op<pegtl::one<'*'>, 5> {};
struct fslash  : infix_op<pegtl::one<'/'>, 5> {};
struct percent : infix_op<pegtl::one<'%'>, 5> {};

struct plus  : infix_op<pegtl::one<'+'>, 6> {};
struct minus : infix_op<pegtl::one<'-'>, 6> {};

struct cmp_lt  : infix_op<pegtl::one<60>, 8> {};        // <
struct cmp_leq : infix_op<pegtl::string<60, 61>, 8> {}; // <=
struct cmp_gt  : infix_op<pegtl::one<62>, 8> {};        // >
struct cmp_geq : infix_op<pegtl::string<62, 61>, 8> {}; // >=

struct cmp_eq  : infix_op<pegtl::two<61>, 9> {};        // ==
struct cmp_neq : infix_op<pegtl::string<33, 61>, 9> {}; // !=

struct op_and  : infix_op<pegtl::sor<pegtl::two<38>, pegtl::keyword<'a', 'n', 'd'>>, 13>  {}; // &&, and
struct op_or : infix_op<pegtl::sor<pegtl::two<124>, pegtl::keyword<'o', 'r'>>, 14> {}; // ||, or

struct braced_expression : pegtl::if_must< pegtl::one<'('>, expression, pegtl::one<')'> > {};

template<> struct lhs_term< 0> : pegtl::sor<atom, braced_expression> {};
template<> struct lhs_term< 2> : pegtl::sor<ops::call, ops::index, lhs_term<1>> {};
template<> struct lhs_term< 3> : pegtl::sor<ops::unary_plus, ops::unary_minus, ops::unary_not, lhs_term<2>> {};

template<> struct infix_term< 4> : pegtl::failure {};
template<> struct infix_term< 5> : pegtl::sor<infix_term<4>, ops::star, ops::fslash, ops::percent> {};
template<> struct infix_term< 6> : pegtl::sor<infix_term<5>, ops::plus, ops::minus> {};
template<> struct infix_term< 8> : pegtl::sor<infix_term<7>, ops::cmp_lt, ops::cmp_leq, ops::cmp_gt, ops::cmp_geq> {};
template<> struct infix_term< 9> : pegtl::sor<infix_term<8>, ops::cmp_eq, ops::cmp_neq> {};
template<> struct infix_term<13> : pegtl::sor<infix_term<12>, ops::op_and> {};
template<> struct infix_term<14> : pegtl::sor<infix_term<13>, ops::op_or> {};

struct infix_expression : infix_term<17> {};
}

struct expression : pegtl::seq<ops::lhs_expression, pegtl::star<ops::infix_expression>> {};

}
}