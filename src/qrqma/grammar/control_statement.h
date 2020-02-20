#pragma once
#include "../tao/pegtl.hpp"
#include "expression.h"
#include "spaces.h"

namespace qrqma {
namespace grammar {
namespace pegtl = tao::pegtl;

struct grammar;
struct text_content;

struct control_statement_o_brackets_raw : pegtl::string<123, 37> {}; // {%
struct control_statement_c_brackets_raw : pegtl::string<37, 125> {}; // %}
struct control_statement_o_brackets : pegtl::sor<pegtl::seq<spaces, control_statement_o_brackets_raw, pegtl::one<'-'>>, control_statement_o_brackets_raw> {};
struct control_statement_c_brackets : pegtl::sor<control_statement_c_brackets_raw, pegtl::seq<pegtl::one<'-'>, control_statement_c_brackets_raw, spaces>> {};

template<typename Token, typename Rule>
struct control_statement_outer : pegtl::if_must<
    pegtl::at<pegtl::seq<control_statement_o_brackets, padded<Token>>>,
    Rule
    > {};
    
template <typename Token, typename... R>
struct control_statement_token
    : pegtl::seq<control_statement_o_brackets,
                 pegtl::until<control_statement_c_brackets,
                              pegtl::if_must<pegtl::seq<padded<Token>, padded<R>...>>>> {};

struct if_token : pegtl::keyword<'i', 'f'> {};

struct conditional_statement : pegtl::seq<pegtl::success, expression> {};
struct if_statement         : control_statement_token<if_token , conditional_statement> {};

struct if_content           : pegtl::seq<pegtl::success, grammar> {};

struct else_statement       : control_statement_token<pegtl::keyword<'e', 'l', 's', 'e'>> {};
struct else_content         : pegtl::seq<pegtl::success, grammar> {};

struct elif_token : pegtl::keyword<'e', 'l', 'i', 'f'> {};
struct elif_statement       : control_statement_token<elif_token , conditional_statement> {};
struct elif_control_statement;

struct else_control_statement : pegtl::sor<
    pegtl::if_must<else_statement, else_content>,
    elif_control_statement> {};

struct elif_control_statement : pegtl::if_must<
    elif_statement, 
    if_content, 
    pegtl::opt<else_control_statement>
    > {};

struct if_control_statement : pegtl::if_must<
    if_statement, 
    if_content, 
    pegtl::opt<else_control_statement>, 
    control_statement_token<pegtl::keyword<'e', 'n', 'd', 'i', 'f'>>
    > {};

struct set_token : pegtl::keyword<'s', 'e', 't'> {};
struct set_identifier : identifier {};
struct set_expression : expression {};
struct set_control_statement : control_statement_token<set_token , set_identifier, pegtl::one<'='>, set_expression> {};

struct for_token : pegtl::keyword<'f', 'o', 'r'> {};
struct for_identifier : identifier {};
struct for_container_identifier : pegtl::seq<pegtl::success, expression> {};
struct for_content : pegtl::seq<pegtl::success, grammar> {};
struct for_control_statement : pegtl::seq<
    control_statement_token<for_token, pegtl::list<for_identifier, padded<pegtl::one<','>>>, pegtl::keyword<'i', 'n'>, for_container_identifier>,
    for_content,
    control_statement_token<pegtl::keyword<'e', 'n', 'd', 'f', 'o', 'r'>>
    > {};


struct extends_token : pegtl::keyword<'e', 'x', 't', 'e', 'n', 'd', 's'> {};
struct extends_control_statement : control_statement_token<extends_token , string_literal> {};

struct block_token : pegtl::keyword<'b', 'l', 'o', 'c', 'k'> {};
struct block_content : pegtl::seq<pegtl::success, grammar> {};
struct block_identifier : identifier {};
struct block_scoped : pegtl::keyword<'s', 'c', 'o', 'p', 'e', 'd'> {};
struct block_end_identifier : identifier {};
struct block_statement : control_statement_token<block_token, block_identifier, pegtl::opt<block_scoped>> {};
struct block_control_statement : pegtl::seq<
    block_statement, 
    block_content,
    control_statement_token<pegtl::keyword<'e', 'n', 'd', 'b', 'l', 'o', 'c', 'k'>, pegtl::opt<block_end_identifier> >
    > {};

struct raw_token : pegtl::keyword<'r', 'a', 'w'> {};
struct raw_end_token : control_statement_token<pegtl::keyword<'e', 'n', 'd', 'r', 'a', 'w'>> {};
struct raw_text : pegtl::until<pegtl::at<raw_end_token>> {};
struct raw_control_statement : pegtl::seq<
    control_statement_token<raw_token>, 
    raw_text,
    raw_end_token
    > {};

struct block_control_statement_outer   : control_statement_outer<block_token, block_control_statement> {};
struct if_control_statement_outer      : control_statement_outer<if_token, if_control_statement> {};
struct set_control_statement_outer     : control_statement_outer<set_token, set_control_statement> {};
struct for_control_statement_outer     : control_statement_outer<for_token, for_control_statement> {};
struct extends_control_statement_outer : control_statement_outer<extends_token, extends_control_statement> {};
struct raw_text_outer                  : control_statement_outer<raw_token, raw_control_statement> {};

struct control_statement : pegtl::sor<
                    block_control_statement_outer,
                    if_control_statement_outer, 
                    set_control_statement_outer, 
                    for_control_statement_outer,
                    extends_control_statement_outer,
                    raw_text_outer
                    > {};

}
}