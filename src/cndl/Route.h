#pragma once

#include "unique_function.h"

#include "Extractor.h"
#include "Request.h"
#include "Response.h"

#include <charconv>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


namespace cndl {

using OptResponse = std::optional<Response>;

struct RouteBase {
    struct Options {
        std::vector<std::string> methods{"GET"};
    };
    RouteBase(Options options) noexcept : m_options{std::move(options)}
    {}

    RouteBase(RouteBase&&) noexcept = default;
    RouteBase& operator=(RouteBase&&) noexcept = default;

    virtual ~RouteBase() = default;
    virtual OptResponse operator()(Request const& request) = 0;

    Options const& getOptions() const {
        return m_options;
    }

protected:
    Options m_options;
};

template <typename T>
struct Route;

template <typename... Args>
struct Route<OptResponse(Request const&, Args...)> : RouteBase {
protected:
    using ParameterTuple = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
    using FuncT = unique_func<OptResponse(Request const&, Args...)>;
    std::regex m_pattern;
    FuncT m_ftor;

    template<std::size_t... indexes>
    OptResponse invoke(Request const& request, ParameterTuple const& params, std::index_sequence<indexes...>) {
        return m_ftor(request, std::get<indexes>(params)...);
    }

    template<int idx, typename MatchResults>
    bool extract(MatchResults const& match_results, ParameterTuple& params) {
        if constexpr (idx < sizeof...(Args)) {
            using ArgType = std::tuple_element_t<idx, ParameterTuple>;
            auto const& sub_match = match_results[idx+1];
            return Extractor<ArgType>::extract(sub_match, std::get<idx>(params)) and
                    extract<idx+1>(match_results, params);
        } else {
            return true;
        }
    }

public:
    using Options = typename RouteBase::Options;

    Route(std::regex pattern, FuncT ftor, Options options={})
      : RouteBase{std::move(options)}
      , m_pattern{std::move(pattern)}
      , m_ftor{std::move(ftor)}
    {
        if (sizeof...(Args) != m_pattern.mark_count()) {
            throw std::invalid_argument("got invalid pattern; expected " +
                 std::to_string(sizeof...(Args)) +
                 " markers but got " + std::to_string(m_pattern.mark_count()));
        }
    }
    Route(Route&& route) noexcept = default;
    Route& operator=(Route&&) noexcept = default;

    Route(std::string pattern, FuncT ftor, Options options={})
      : Route(std::regex{pattern}, std::move(ftor), std::move(options))
    {}

    virtual ~Route() = default;

    OptResponse operator()(Request const& request) override {
        auto resource = std::string_view{request.header.resource};
        std::match_results<decltype(std::begin(resource))> res;
        bool success = std::regex_match(begin(resource), end(resource), res, m_pattern);
        if (success) {
            if (std::find(begin(m_options.methods), end(m_options.methods), request.header.method) == std::end(m_options.methods)) {
                throw Error(405);
            }
            ParameterTuple args;
            if (extract<0>(res, args)) {
                return invoke(request, args, std::index_sequence_for<Args...>());
            }
        }
        return {};
    }
};

template <typename _Functor,
          typename _Signature = typename detail::__function_guide_helper<
              decltype(&_Functor::operator())>::type>
Route(std::regex, _Functor, RouteBase::Options={})->Route<_Signature>;

template <typename T>
struct GlobalRoute;

template <typename... Args>
struct GlobalRoute<OptResponse(Request const&, Args...)> : Route<OptResponse(Request const&, Args...)> {
    using SuperClass = Route<OptResponse(Request const&, Args...)>;
    using FuncT = typename SuperClass::FuncT;
    using Options = typename SuperClass::Options;

    GlobalRoute(std::regex pattern, FuncT ftor, Options options={})
      : SuperClass(std::move(pattern), std::move(ftor), std::move(options))
    {
        registerRouteGlobally(*this);
    }

    GlobalRoute(std::string pattern, FuncT ftor, Options options={})
      : GlobalRoute(std::regex{pattern}, std::move(ftor), std::move(options))
    {}

    virtual ~GlobalRoute() {
        deregisterRouteGlobally(*this);
    };
};

template <typename T, typename _Functor,
          typename _Signature = typename detail::__function_guide_helper<
              decltype(&_Functor::operator())>::type>
GlobalRoute(T, _Functor, RouteBase::Options={})->GlobalRoute<_Signature>;

void registerRouteGlobally(RouteBase&);
void deregisterRouteGlobally(RouteBase&);

}
