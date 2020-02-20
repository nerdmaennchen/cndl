#pragma once

#include <utility>

namespace qrqma {
namespace internal {

template<typename T>
struct Finally final {
    std::optional<T> opt_expr;
    Finally(T&& expr) : opt_expr{std::forward<T>(expr)} {}

    void disarm() { opt_expr.reset(); }

    ~Finally() {
        if (opt_expr) { 
            (*opt_expr)();
        }
    }
};

template<typename T>
Finally(T&& expr) -> Finally<T>;
   
}
}