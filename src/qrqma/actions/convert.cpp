#include "convert.h"
#include "types.h"
#include "../demangle.h"

#include <numeric>
#include <charconv>

namespace qrqma {
namespace actions {

namespace {

template<typename T>
std::optional<std::any> trivial_cast(std::any const& in) {
    if (typeid(bool) == in.type()) {
        return static_cast<T>(std::any_cast<bool>(in));
    } else if (typeid(std::int64_t) == in.type()) {
        return static_cast<T>(std::any_cast<int64_t>(in));
    } else if (typeid(std::uint64_t) == in.type()) {
        return static_cast<T>(std::any_cast<uint64_t>(in));
    } else if (typeid(std::int32_t) == in.type()) {
        return static_cast<T>(std::any_cast<int32_t>(in));
    } else if (typeid(std::uint32_t) == in.type()) {
        return static_cast<T>(std::any_cast<uint32_t>(in));
    } else if (typeid(std::int16_t) == in.type()) {
        return static_cast<T>(std::any_cast<int16_t>(in));
    } else if (typeid(std::uint16_t) == in.type()) {
        return static_cast<T>(std::any_cast<uint16_t>(in));
    } else if (typeid(std::int8_t) == in.type()) {
        return static_cast<T>(std::any_cast<int8_t>(in));
    } else if (typeid(std::uint8_t) == in.type()) {
        return static_cast<T>(std::any_cast<uint8_t>(in));
    } else if (typeid(double) == in.type()) {
        return static_cast<T>(std::any_cast<double>(in));
    } else if (typeid(float) == in.type()) {
        return static_cast<T>(std::any_cast<float>(in));
    }
    return {};
}

template<typename T, typename Func>
std::optional<std::any> nontrivial_cast(std::any const& in, Func&& f) {
    if (typeid(bool) == in.type()) {
        return static_cast<T>(f(std::any_cast<bool>(in)));
    } else if (typeid(std::int64_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<int64_t>(in)));
    } else if (typeid(std::uint64_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<uint64_t>(in)));
    } else if (typeid(std::int32_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<int32_t>(in)));
    } else if (typeid(std::uint32_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<uint32_t>(in)));
    } else if (typeid(std::int16_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<int16_t>(in)));
    } else if (typeid(std::uint16_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<uint16_t>(in)));
    } else if (typeid(std::int8_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<int8_t>(in)));
    } else if (typeid(std::uint8_t) == in.type()) {
        return static_cast<T>(f(std::any_cast<uint8_t>(in)));
    } else if (typeid(double) == in.type()) {
        return static_cast<T>(f(std::any_cast<double>(in)));
    } else if (typeid(float) == in.type()) {
        return static_cast<T>(f(std::any_cast<float>(in)));
    }
    return {};
}

}

std::any convert(std::any const& in, std::type_info const& to) {
    if (in.type() == to) {
        return in;
    }

    if (typeid(types::String) == to) {
        if (typeid(void) == in.type()) {
            return std::string{};
        } else if (typeid(types::Undefined) == in.type()) {
            return std::string{};
        } else if (typeid(symbol::List) == in.type()) {
            auto const& l = std::any_cast<symbol::List const&>(in);
            std::string ret = "[";
            auto to_str = [] (std::any const& e) {
                return convert<std::string>(e);
            };
            if (not l.empty()) {
                ret += std::accumulate(std::next(std::begin(l)), std::end(l), to_str(*std::begin(l)), [=](std::string const& l, std::any const& r) {
                    return l + ", " + to_str(r);
                });
            }
            ret += "]";
            return ret;
        } else if (typeid(symbol::Map) == in.type()) {
            auto to_str = [] (std::pair<std::string, std::any> const& p) {
                return "\"" + p.first + "\": " + convert<std::string>(p.second);
            };
            auto const& m = std::any_cast<symbol::Map const&>(in);
            std::string ret = "{";
            if (not m.empty()) {
                ret += std::accumulate(std::next(std::begin(m)), std::end(m), to_str(*std::begin(m)), [=](std::string const& l, auto const& p) {
                    return l + ", " + "\"" + p.first + "\": " + convert<std::string>(p.second);
                });
            }
            ret += "}";
            return ret;
        } else {
            auto cast = nontrivial_cast<types::String>(in, [](auto v) { return std::to_string(v); });
            if (cast) {
                return *cast;
            }
        }
    } else if (typeid(types::Bool) == to) {
        if (typeid(std::string) == in.type()) {
            return not std::any_cast<std::string const&>(in).empty();
        } else if (typeid(types::Undefined) == in.type()) {
            return false;
        } else if (typeid(void) == in.type()) {
            return false;
        } else if (typeid(types::List) == in.type()) {
            return not std::any_cast<types::List const&>(in).empty();
        } else if (typeid(types::Map) == in.type()) {
            return not std::any_cast<types::Map const&>(in).empty();
        } else {
            auto cast = trivial_cast<types::Bool>(in);
            if (cast) {
                return *cast;
            }
        }
    } else if (typeid(types::Integer) == to) {
        if (typeid(std::string) == in.type()) {
            std::string const& s = std::any_cast<std::string const&>(in);
            types::Integer val{};
            auto conv_res = std::from_chars(s.data(), s.data()+s.size(), val, 0);
            if (conv_res.ec == std::errc::invalid_argument) {
                throw std::runtime_error("cannot convert str \"" + s + "\" to Integer");
            }
            return val;
        } else {
            auto cast = trivial_cast<types::Integer>(in);
            if (cast) {
                return *cast;
            }
        }
    } else if (typeid(std::int64_t) == to) {
        auto cast = trivial_cast<std::int64_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::uint64_t) == to) {
        auto cast = trivial_cast<std::uint64_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::int32_t) == to) {
        auto cast = trivial_cast<std::int32_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::uint32_t) == to) {
        auto cast = trivial_cast<std::uint32_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::int16_t) == to) {
        auto cast = trivial_cast<std::int16_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::uint16_t) == to) {
        auto cast = trivial_cast<std::uint16_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::int8_t) == to) {
        auto cast = trivial_cast<std::int8_t>(in);
        if (cast) {
            return *cast;
        }
    } else if (typeid(std::uint8_t) == to) {
        auto cast = trivial_cast<std::uint8_t>(in);
        if (cast) {
            return *cast;
        }
    }
    throw std::runtime_error("dont know how to create a converter from type " + internal::demangle(in.type()) + " to " + internal::demangle(to));
}

}
}