#pragma once

#include "request.h"
#include "websocket.h"
#include "arg_extractors.h"
#include "unique_function.h"

#include <type_traits>
#include <chrono>
#include <regex>

namespace cndl {

namespace detail {

template<typename Signature>
struct SignatureCheckerDetail {
    static void check(std::regex const&) {}
    using ParameterTuple = std::tuple<>;
    using has_on_open = std::false_type;
};
template<typename... Args>
struct SignatureCheckerDetail<bool(Request const&, Websocket&, Args...)> {
    using ParameterTuple = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
    using has_on_open = std::true_type;

    static void check(std::regex const& pattern) {
        if (sizeof...(Args) != pattern.mark_count()) {
            throw std::invalid_argument("got invalid pattern; expected " +
                std::to_string(sizeof...(Args)) +
                " markers but got " + std::to_string(pattern.mark_count()));
        }
    }

    template<int idx, typename MatchResults>
    static bool extract(MatchResults const& match_results, ParameterTuple& params) {
        if constexpr (idx < sizeof...(Args)) {
            using ArgType = std::tuple_element_t<idx, ParameterTuple>;
            auto const& sub_match = match_results[idx+1];
            return Extractor<ArgType>::extract(sub_match, std::get<idx>(params)) and
                    extract<idx+1>(match_results, params);
        } else {
            return true;
        }
    }

    template<typename T, std::size_t... indexes>
    static bool invoke(T& t, Request const& request, Websocket& ws, ParameterTuple const& params, std::index_sequence<indexes...>) {
        return t.onOpen(request, ws, std::get<indexes>(params)...);
    }

    template<typename T>
    static bool invoke(T& t, Request const& request, Websocket& ws, ParameterTuple const& params) {
        return invoke(t, request, ws, params, std::index_sequence_for<Args...>());
    }
};

template <typename T, typename Signature = typename detail::__function_guide_helper<decltype(&T::onOpen)>::type>
using SignatureChecker = SignatureCheckerDetail<Signature>;

}

struct WSRouteBase {
    virtual bool operator()([[maybe_unused]] Request const& request, [[maybe_unused]] Websocket& ws) = 0;
    WSRouteBase(WebsocketHandler* handler) : m_handler{handler} {}

    WebsocketHandler* getHandler() {
        return m_handler;
    }
private:
    WebsocketHandler* m_handler;
};

template <typename T>
struct WSRoute : WSRouteBase {
    static_assert(std::is_base_of_v<WebsocketHandler, T>, "T must be derived from WebsocketHandler!");
protected:
    using SignatureChecker = detail::SignatureChecker<T>; 
    using ParameterTuple = typename SignatureChecker::ParameterTuple; 
    std::regex m_pattern;

    T& handler;
public:
    virtual ~WSRoute() = default;

    bool operator()(Request const& request, Websocket& websocket) override {
        auto ressource = std::string_view{request.header.ressource};
        std::match_results<decltype(std::begin(ressource))> res;
        bool success = std::regex_match(begin(ressource), end(ressource), res, m_pattern);
        if (success) {
            if constexpr (SignatureChecker::has_on_open::value) {
                ParameterTuple args;
                if (SignatureChecker::template extract<0>(res, args)) {
                    return SignatureChecker::template invoke(handler, request, websocket, args);
                }
            }
        }
        return success;
    }

    WSRoute(std::regex pattern, T& handler)
      : WSRouteBase{&handler}
      , m_pattern{std::move(pattern)}
      , handler{handler}
    {}

    WSRoute(std::string pattern, T& handler)
      : WSRoute(std::regex{pattern}, handler) 
    {}
};

template <typename _Functor,
          typename _Signature = typename detail::__function_guide_helper<
              decltype(&_Functor::operator())>::type>
WSRoute(std::regex, _Functor)->WSRoute<_Functor>;

// template <typename T> void registerRouteGlobally(WSRouteBase*);

}