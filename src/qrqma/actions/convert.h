#pragma once

#include <typeinfo>
#include <functional>
#include <any>

namespace qrqma {
namespace actions {

// a function that eats an any and converts the inner type to something else
std::any convert(std::any const& in, std::type_info const& to);

template<typename T>
T convert(std::any const& a) {
    using T_ = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::any_cast<T>(convert(a, typeid(T_)));
}

}
}