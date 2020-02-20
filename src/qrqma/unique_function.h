#pragma once
#include <functional>
#include <memory>

namespace qrqma {

template <typename T> struct unique_func;

template <typename Res, typename... Args> struct unique_func<Res(Args...)> {
private:
    struct PimplBase {
        PimplBase() = default;
        virtual ~PimplBase() = default;
        virtual Res invoke(Args &&... args) = 0;
        virtual std::type_info const &target_type() const = 0;
    };

    template <typename Functor> struct TypedPimpl : PimplBase {
        Functor ftor;
        TypedPimpl(Functor _ftor) : ftor{std::move(_ftor)} {}
        Res invoke(Args &&... args) override {
            return std::invoke(ftor, std::forward<Args>(args)...);
        }
        std::type_info const &target_type() const override {
            return typeid(Functor);
        }
    };
    std::unique_ptr<PimplBase> pimpl;

public:
    template <typename Functor>
    unique_func(Functor functor)
        : pimpl{std::make_unique<TypedPimpl<Functor>>(std::move(functor))} {}

    unique_func() = default;
    unique_func(unique_func &&) = default;
    unique_func &operator=(unique_func &&) = default;
    unique_func(unique_func const &) = delete;
    unique_func &operator=(unique_func const&) = delete;

    Res operator()(Args &&... args) {
        return pimpl->invoke(std::forward<Args>(args)...);
    }
    Res operator()(Args &&... args) const {
        return pimpl->invoke(std::forward<Args>(args)...);
    }

    std::type_info const &target_type() const { return pimpl->target_type(); }
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
struct __function_guide_helper<_Res (_Tp::*)(_Args...) const &noexcept(_Nx)> {
    using type = _Res(_Args...);
};

} // namespace detail

template <typename _Functor,
          typename _Signature = typename detail::__function_guide_helper<
              decltype(&_Functor::operator())>::type>
unique_func(_Functor)->unique_func<_Signature>;

} // namespace qrqma