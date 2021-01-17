#pragma once
#include <utility>
#include <memory>

namespace cndl {

template <typename T> struct unique_func;

template <typename Res, typename... Args> struct unique_func<Res(Args...)> {
private:
    struct PimplBase {
        virtual ~PimplBase() = default;
        virtual Res invoke(Args &&... args) const = 0;
    };

    template <typename Functor> struct TypedPimpl : PimplBase {
        Functor ftor;
        TypedPimpl(Functor _ftor) : ftor{std::move(_ftor)} {}
        Res invoke(Args &&... args) const override {
            return ftor(std::forward<Args>(args)...);
        }
    };
    std::unique_ptr<PimplBase> pimpl;

public:
    template <typename Functor>
    unique_func(Functor functor)
        : pimpl{std::make_unique<TypedPimpl<Functor>>(std::move(functor))} {}

    unique_func() noexcept = default;
    unique_func(unique_func &&) noexcept = default;
    unique_func &operator=(unique_func &&) noexcept = default;
    unique_func(unique_func const &) = delete;
    unique_func &operator=(unique_func const&) = delete;

    operator bool() const noexcept {
        return static_cast<bool>(pimpl);
    }

    Res operator()(Args... args) const noexcept(noexcept(pimpl->invoke(std::forward<Args>(args)...))) {
        return pimpl->invoke(std::forward<Args>(args)...);
    }
};

namespace detail {

template <typename> struct __function_guide_helper {};

template <typename _Res, typename _Tp, bool _Nx, typename... _Args>
struct __function_guide_helper<_Res (_Tp::*)(_Args...) noexcept(_Nx)> {
    using type = _Res(_Args...);
};

template <typename _Res, typename _Tp, bool _Nx, typename... _Args>
struct __function_guide_helper<_Res (_Tp::*)(_Args...) & noexcept(_Nx)> {
    using type = _Res(_Args...);
};

template <typename _Res, typename _Tp, bool _Nx, typename... _Args>
struct __function_guide_helper<_Res (_Tp::*)(_Args...) const noexcept(_Nx)> {
    using type = _Res(_Args...);
};

template <typename _Res, typename _Tp, bool _Nx, typename... _Args>
struct __function_guide_helper<_Res (_Tp::*)(_Args...) const & noexcept(_Nx)> {
    using type = _Res(_Args...);
};

} // namespace detail

template <typename _Functor,
          typename _Signature = typename detail::__function_guide_helper<
              decltype(&_Functor::operator())>::type>
unique_func(_Functor)->unique_func<_Signature>;

} // namespace cndl
