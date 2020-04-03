#pragma once

#include <stdexcept>

namespace cndl {

struct Error : std::runtime_error {
    Error(int code, char const* message)
    : std::runtime_error(message)
    , m_code{code}
    {}

    Error(int code) : Error(code, "") {}

    int code() const {
        return m_code;
    }
private:
    int m_code;
};

}
