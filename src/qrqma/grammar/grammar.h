#pragma once
#include "../tao/pegtl.hpp"
#include "spaces.h"
#include "expression.h"
#include "control_statement.h"

namespace qrqma {
namespace grammar {

struct grammar;
namespace pegtl = tao::pegtl;

struct print_expression_o_brackets : pegtl::string<123, 123> {}; // {{
struct print_expression_c_brackets : pegtl::string<125, 125> {}; // }}

struct comment_o_brackets : pegtl::string<123, 35> {}; // {#
struct comment_c_brackets : pegtl::string<35, 125> {}; // #}
struct comment
    : pegtl::if_must<comment_o_brackets, pegtl::until<comment_c_brackets>> {};

struct print_expression
    : pegtl::if_must<print_expression_o_brackets,
                     pegtl::until<print_expression_c_brackets, expression>> {};

struct any_opening_brackets
    : pegtl::sor<print_expression_o_brackets, control_statement_o_brackets,
                 comment_o_brackets> {};

struct text
    : pegtl::until<pegtl::at<pegtl::sor<pegtl::eof, any_opening_brackets>>> {};

struct control_text : pegtl::sor<print_expression, control_statement, comment> {};

struct grammar : pegtl::list<text, control_text> {};

} // namespace grammar
} // namespace qrqma
