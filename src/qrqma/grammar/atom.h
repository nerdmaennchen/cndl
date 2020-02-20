#pragma once
#include "../tao/pegtl.hpp"
#include "spaces.h"

namespace qrqma {
namespace grammar {
namespace pegtl = tao::pegtl;

struct atom;
struct expression;

template <char delim>
struct quoted_string : pegtl::if_must<pegtl::one<delim>, pegtl::until<pegtl::one<delim>>> {};
struct string_literal : pegtl::sor<quoted_string<39>, quoted_string<34>> {}; // a string enclosed in '' or ""

struct digits : pegtl::plus<pegtl::digit> {};
struct integer : digits {};

struct bool_true  : pegtl::sor<pegtl::keyword<'T', 'r', 'u', 'e'>, pegtl::keyword<'t', 'r', 'u', 'e'>> {};
struct bool_false : pegtl::sor<pegtl::keyword<'F', 'a', 'l', 's', 'e'>, pegtl::keyword<'f', 'a', 'l', 's', 'e'>> {};
struct boolean : pegtl::sor<bool_true, bool_false> {};

struct float_exponent : pegtl::seq<pegtl::one<'e', 'E'>, pegtl::opt<pegtl::one<'-'>>, digits> {};
struct float_num : pegtl::seq<
                    pegtl::star<pegtl::digit>,
                    pegtl::sor<
                        pegtl::seq<pegtl::one<'.'>, pegtl::star<pegtl::digit>, pegtl::opt<float_exponent>>,
                        float_exponent
                    >> {};
struct number : pegtl::sor<float_num, integer> {};

// similar to pegtl::identifier_other but with the '.' character allowed
using identifier_other = pegtl::internal::ranges< pegtl::internal::peek_char, 'a', 'z', 'A', 'Z', '0', '9', '_', '_', '.' >;
struct identifier : pegtl::seq<pegtl::not_at<boolean>, pegtl::seq< pegtl::identifier_first, pegtl::star< identifier_other > >> {};

struct atom_list : pegtl::if_must<pegtl::one<'['>, pegtl::opt<pegtl::list<padded<expression>, pegtl::one<','>>>, spaces, pegtl::one<']'>> {};

struct map_elem : pegtl::seq<padded<atom>, pegtl::one<':'>, padded<expression>> {};
struct atom_map : pegtl::if_must<pegtl::one<123>, pegtl::opt<pegtl::list<map_elem, pegtl::one<','>>>, spaces, pegtl::one<125>> {};

struct atom : pegtl::sor<string_literal, boolean, number, identifier, atom_list, atom_map> {};


}
}