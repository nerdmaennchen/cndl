#pragma once

#include <string>
#include <typeinfo>

namespace qrqma::internal {

std::string demangle(std::type_info const &ti);

}