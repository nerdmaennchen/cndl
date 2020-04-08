#pragma once

#include "Extractor.h"
#include "Request.h"
#include "Websocket.h"
#include "unique_function.h"

#include <chrono>
#include <regex>
#include <type_traits>

namespace cndl {

namespace detail {


template<typename... Args>
struct ArgExtractorDetail {
    using ParameterTuple = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;

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
};

template<typename T>
struct SignatureCheckerDetail_canOpen;
template<typename... Args>
struct SignatureCheckerDetail_canOpen<bool(Request const&, Args...)> {
    using ArgExtractor = ArgExtractorDetail<Args...>;
    using ParameterTuple = typename ArgExtractor::ParameterTuple;
    using has_canOpen = std::true_type;

    template<typename T, std::size_t... indexes>
    static bool invoke(T& t, Request const& request, ParameterTuple const& params, std::index_sequence<indexes...>) {
        return t.canOpen(request, std::get<indexes>(params)...);
    }

    template<typename T>
    static bool invoke(T& t, Request const& request, ParameterTuple const& params) {
        return invoke(t, request, params, std::index_sequence_for<Args...>());
    }
};

template<typename T>
struct SignatureCheckerDetail_onOpen;

template<typename... Args>
struct SignatureCheckerDetail_onOpen<void(Request const&, Websocket&, Args...)> {
    using ArgExtractor = ArgExtractorDetail<Args...>;
    using ParameterTuple = typename ArgExtractor::ParameterTuple;
    using has_onOpen = std::true_type;

    template<typename T, std::size_t... indexes>
    static void invoke(T& t, Request const& request, Websocket& ws, ParameterTuple const& params, std::index_sequence<indexes...>) {
        t.onOpen(request, ws, std::get<indexes>(params)...);
    }

    template<typename T>
    static void invoke(T& t, Request const& request, Websocket& ws, ParameterTuple const& params) {
        invoke(t, request, ws, params, std::index_sequence_for<Args...>());
    }
};

template <typename T>
struct has_canOpen_function {
private:
	template <typename U>
	static decltype(&U::canOpen, void(), std::true_type()) test(int);
	template <typename>
	static std::false_type test(...);
public:
	using type = decltype(test<T>(int(0)));
	enum { value = type::value };
};
template <typename T>
inline constexpr bool has_canOpen_function_v = has_canOpen_function<T>::value;

template <typename T>
struct has_onOpen_function {
private:
	template <typename U>
	static decltype(&U::onOpen, void(), std::true_type()) test(int);
	template <typename>
	static std::false_type test(...);
public:
	using type = decltype(test<T>(int(0)));
	enum { value = type::value };
};
template <typename T>
inline constexpr bool has_onOpen_function_v = has_onOpen_function<T>::value;

template <typename T, typename Signature = typename detail::__function_guide_helper<decltype(&T::onOpen)>::type>
using SignatureChecker_onOpen = SignatureCheckerDetail_onOpen<Signature>;

template <typename T, typename Signature = typename detail::__function_guide_helper<decltype(&T::canOpen)>::type>
using SignatureChecker_canOpen = SignatureCheckerDetail_canOpen<Signature>;

}

struct WSRouteBase {
    virtual bool canOpen([[maybe_unused]] Request const& request) = 0;
    virtual void onOpen(Request const& request, Websocket& ws) = 0;
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
    std::regex m_pattern;

    T& handler;
public:
    virtual ~WSRoute() = default;

    bool canOpen([[maybe_unused]] Request const& request) override {
        auto resource = std::string_view{request.header.resource};
        std::match_results<decltype(std::begin(resource))> res;
        bool success = std::regex_match(begin(resource), end(resource), res, m_pattern);
        if (success) {
            if constexpr (detail::has_canOpen_function_v<T>) {
                using SignatureChecker = detail::SignatureChecker_canOpen<T>;
                using ArgExtractor = typename SignatureChecker::ArgExtractor;
                typename SignatureChecker::ParameterTuple args;
                if (ArgExtractor::template extract<0>(res, args)) {
                    return SignatureChecker::template invoke(handler, request, args);
                }
            }
        }
        return success;
    }

    void onOpen(Request const& request, Websocket& ws) override {
        auto resource = std::string_view{request.header.resource};
        std::match_results<decltype(std::begin(resource))> res;
        bool success = std::regex_match(begin(resource), end(resource), res, m_pattern);
        if (success) {
            if constexpr (detail::has_onOpen_function_v<T>) {
                using SignatureChecker = detail::SignatureChecker_onOpen<T>;
                using ArgExtractor = typename SignatureChecker::ArgExtractor;
                typename SignatureChecker::ParameterTuple args;
                if (ArgExtractor::template extract<0>(res, args)) {
                    return SignatureChecker::template invoke(handler, request, ws, args);
                }
            }
        } else {
            throw std::invalid_argument("cannot call onOpen with incorrect request");
        }
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

template <typename T>
struct GlobalWSRoute : WSRoute<T> {
    GlobalWSRoute(std::regex pattern, T& handler)
      : WSRoute<T>(std::move(pattern), handler)
    {
        registerRouteGlobally(*this);
    }

    GlobalWSRoute(std::string pattern, T& handler)
      : GlobalWSRoute(std::regex{pattern}, handler)
    {}

    virtual ~GlobalWSRoute() {
        deregisterRouteGlobally(*this);
    }
};

void registerRouteGlobally(WSRouteBase& route);
void deregisterRouteGlobally(WSRouteBase& route);

}
