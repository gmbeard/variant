#ifndef VARIANT_VARIANT_HPP_INCLUDED
#define VARIANT_VARIANT_HPP_INCLUDED

#include <algorithm>
#include <cassert>
#include <type_traits>

namespace variant {

    template<typename... Ts>
    constexpr auto max_size() -> size_t {
        return std::max({ sizeof(Ts)... });
    }

    template<typename... Ts>
    constexpr auto max_align() -> size_t {
        return std::max({ alignof(Ts)... });
    }

    template<size_t N, typename U, typename... Ts>
    struct type_index_of;

    template<size_t N, typename U, typename V, typename... Ts>
    struct type_index_of<N, U, V, Ts...> {
        static constexpr size_t value = 
            type_index_of<N+1, U, Ts...>::value;
    };

    template<size_t N, typename U, typename... Ts>
    struct type_index_of<N, U, U, Ts...> {
        static constexpr size_t value = N;
    };

    template<size_t N, typename U>
    struct type_index_of<N, U> { };

    template<typename... Ts>
    struct all_noexcept_move_constructible;

    template<typename T, typename... Ts>
    struct all_noexcept_move_constructible<T, Ts...> {
        static constexpr bool value = 
            noexcept(T { std::declval<T>() }) &&
                all_noexcept_move_constructible<Ts...>::value;
    };

    template<>
    struct all_noexcept_move_constructible<> {
        static constexpr bool value = true;
    };

    template<typename... Ts>
    struct all_noexcept_copy_constructible;

    template<typename T, typename... Ts>
    struct all_noexcept_copy_constructible<T, Ts...> {
        static constexpr bool value = 
            noexcept(T { std::declval<T const&>() }) &&
                all_noexcept_copy_constructible<Ts...>::value;
    };

    template<>
    struct all_noexcept_copy_constructible<> {
        static constexpr bool value = true;
    };

    template<typename T, typename F, typename S, typename... Ts>
    auto apply_visitor(F&& f, S storage, Ts&&... args) {
        return 
            std::forward<F>(f)(*reinterpret_cast<T*>(storage), std::forward<Ts>(args)...);
    }

    template<typename... Ts>
    struct Variant {

        static_assert(0 < sizeof...(Ts),
            "Variant must hold at least one type");

        template<
            typename U,
            typename std::enable_if<
                !std::is_same<
                    typename std::decay<U>::type,
                    Variant>::value>::type* = nullptr>
        explicit Variant(U&& val)
            noexcept(noexcept(typename std::decay<U>::type { std::declval<U>() }))
        :
            type_index_ { 
                type_index_of<0, typename std::decay<U>::type, Ts...>::value 
            } 
        {
            new (storage_) typename std::decay<U>::type { std::forward<U>(val) };
        }

        Variant(Variant const& other)
            noexcept(all_noexcept_move_constructible<Ts...>::value)
        :
            type_index_ { other.type_index_ }
        {
            visit(
                [this](auto const& val) {
                    using T = typename std::decay<decltype(val)>::type;
                    new (storage_) T { val };
                },
                other
            );
        }

        ~Variant() {
            visit(
                [](auto& val) {
                    using T = typename std::decay<decltype(val)>::type;
                    val.~T();
                },
                *this
            );
        }

        Variant& operator=(Variant const&) = delete;

        template<typename T, typename... Ts>
        friend auto is_alternative(Variant<Ts...> const&) -> bool;

        template<typename F, typename T, typename... Ts>
        friend auto visit(F&&, Variant<T, Ts...> const&) 
            -> typename std::result_of<F(T const&)>::type;

        template<typename F, typename T, typename... Ts>
        friend auto visit(F&&, Variant<T, Ts...>&) 
            -> typename std::result_of<F(T&)>::type;

    private:

        using Storage = 
            typename std::aligned_storage<max_size<Ts...>(),
                                           max_align<Ts...>()>::type;
        auto get_storage() const -> Storage const* {
            return &storage_[0];
        }

        auto get_storage() -> Storage* {
            return &storage_[0];
        }

        size_t type_index_;
        Storage storage_[1];
    };

    template<typename T, typename... Ts>
    auto is_alternative(Variant<Ts...> const& v) -> bool {
        return type_index_of<0, T, Ts...>::value == v.type_index_;
    }

    template<typename F, typename T, typename... Ts>
    auto visit(F&& visitor, Variant<T, Ts...> const& var) 
        -> typename std::result_of<F(T const&)>::type
    {
        using Fr = typename std::add_rvalue_reference<F>::type;
        using Fn = void (*)(Fr, decltype(var.get_storage()));
        Fn paths[sizeof...(Ts) + 1] = {
            apply_visitor<T const, Fr, decltype(var.get_storage())>,
            apply_visitor<Ts const, Fr, decltype(var.get_storage())>...
        };

        return (paths[var.type_index_])(
            std::forward<F>(visitor), 
            var.get_storage());
    }

    template<typename F, typename T, typename... Ts>
    auto visit(F&& visitor, Variant<T, Ts...>& var) 
        -> typename std::result_of<F(T&)>::type
    {
        using Fr = typename std::add_rvalue_reference<F>::type;
        using Fn = void (*)(Fr, decltype(var.get_storage()));
        Fn paths[sizeof...(Ts) + 1] = {
            apply_visitor<T, Fr, decltype(var.get_storage())>,
            apply_visitor<Ts, Fr, decltype(var.get_storage())>...
        };

        return (paths[var.type_index_])(
            std::forward<F>(visitor), 
            var.get_storage());
    }
}
#endif //VARIANT_VARIANT_HPP_INCLUDED
