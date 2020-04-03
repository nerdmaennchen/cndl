#pragma once

#include <charconv>
#include <string>
#include <string_view>
#include <utility>

namespace cndl {

template<typename T>
struct Extractor;

template<>
struct Extractor<std::string_view> {
    template<typename IterPairT>
    static bool extract(IterPairT in, std::string_view& out) noexcept {
        out = std::string_view{in.first, std::size_t(in.second-in.first)};
        return true;
    }
};

template<>
struct Extractor<std::string> {
    template<typename IterPairT>
    static bool extract(IterPairT in, std::string& out) noexcept {
        out = std::string{in.first, in.second};
        return true;
    }
};

template<>
struct Extractor<int> {
    template<typename IterPairT>
    static bool extract(IterPairT in, int& out) noexcept {
        auto res = std::from_chars(in.first, in.second, out);
        if (res.ptr != in.second or res.ec == std::errc::invalid_argument) {
            return false;
        }
        return true;
    }
};

}
