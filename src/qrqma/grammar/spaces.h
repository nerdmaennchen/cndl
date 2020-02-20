#pragma once
#include "../tao/pegtl.hpp"

namespace qrqma {
namespace grammar {

namespace pegtl = tao::pegtl;

template <typename T> struct padded : pegtl::pad<T, pegtl::space> {};
struct spaces : pegtl::star<pegtl::space> {};

}
}