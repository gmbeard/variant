#ifndef VARIANT_VARIANT_HPP_INCLUDED
#define VARIANT_VARIANT_HPP_INCLUDED

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <stdexcept>
#include <tuple>

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
    struct all_move_constructible;

    template<typename T, typename... Ts>
    struct all_move_constructible<T, Ts...> {
        static constexpr bool value = 
            std::is_move_constructible<T>::value &&
                all_move_constructible<Ts...>::value;
    };

    template<>
    struct all_move_constructible<> {
        static constexpr bool value = true;
    };

    template<typename... Ts>
    struct all_copy_constructible;

    template<typename T, typename... Ts>
    struct all_copy_constructible<T, Ts...> {
        static constexpr bool value = 
            std::is_copy_constructible<T>::value &&
                all_copy_constructible<Ts...>::value;
    };

    template<>
    struct all_copy_constructible<> {
        static constexpr bool value = true;
    };

    template<typename... Ts>
    struct all_noexcept_move_constructible;

    template<typename T, typename... Ts>
    struct all_noexcept_move_constructible<T, Ts...> {
        static constexpr bool value = 
            std::is_nothrow_move_constructible<T>::value &&
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
            std::is_nothrow_copy_constructible<T>::value &&
                all_noexcept_copy_constructible<Ts...>::value;
    };

    template<>
    struct all_noexcept_copy_constructible<> {
        static constexpr bool value = true;
    };

    template<size_t I, typename... Ts>
    using type_at_index_t = std::tuple_element_t<I, std::tuple<Ts...>>;

    template<typename... Ts>
    using first_type_t = type_at_index_t<0, Ts...>;

    template<typename T, typename F, typename S, typename... Ts>
    decltype(auto) apply_visitor(F&& f, S storage, Ts&&... args) {
        return 
            std::forward<F>(f)(
                *reinterpret_cast<T*>(storage), 
                std::forward<Ts>(args)...
            );
    }

    template<typename T, typename F, typename S, typename... Ts>
    decltype(auto) apply_move_visitor(F&& f, S storage, Ts&&... args) {
        return 
            std::forward<F>(f)(
                std::move(*reinterpret_cast<T*>(storage)), 
                std::forward<Ts>(args)...
            );
    }

    struct IncorrectAlternativeError : std::runtime_error {
        IncorrectAlternativeError() :
            std::runtime_error("Attempted to access incorrect alternative")
        { }
    };

    struct NoCopyable {
        NoCopyable() = default;
        NoCopyable(NoCopyable const&) = delete;
        NoCopyable(NoCopyable&&) = default;
        NoCopyable& operator=(NoCopyable const&) = delete;
        NoCopyable& operator=(NoCopyable&&) = default;
    protected:
        ~NoCopyable() = default;
    };

    struct Copyable {
        Copyable() = default;
        Copyable(Copyable const&) = default;
        Copyable(Copyable&&) = default;
        Copyable& operator=(Copyable const&) = default;
        Copyable& operator=(Copyable&&) = default;
    protected:
        ~Copyable() = default;
    };

    template<typename... Ts>
    struct VariantStorage {
        template<
            typename U,
            typename std::enable_if<
                !std::is_same<
                    typename std::decay<U>::type,
                    VariantStorage>::value>::type* = nullptr>
        VariantStorage(U&& val)
            noexcept(
                noexcept(typename std::decay<U>::type { std::declval<U>() })
            )
        :
            type_index_ { 
                type_index_of<0, typename std::decay<U>::type, Ts...>::value 
            } 
        {
            new (get_storage()) typename std::decay<U>::type { 
                std::forward<U>(val) 
            };
        }

        VariantStorage(VariantStorage const& other)
            noexcept(all_noexcept_copy_constructible<Ts...>::value)
        :
            type_index_ { other.type_index_ }
        {
            other.visit(
                [this](auto const& val) {
                    using T = typename std::decay<decltype(val)>::type;
                    new (static_cast<void*>(get_storage())) T { val };
                }
            );
        }

        VariantStorage(VariantStorage&& other)
            noexcept(all_noexcept_move_constructible<Ts...>::value)
        :
            type_index_ { other.type_index_ }
        {
            std::move(other).visit(
                [this](auto&& val) {
                    static_assert(
                        !std::is_const<
                            std::remove_reference_t<decltype(val)>>::value,
                        "Cannot be const");
                    using T = typename std::decay<decltype(val)>::type;
                    new (static_cast<void*>(get_storage())) T { 
                        std::move(val) 
                    };
                }
            );
        }

        ~VariantStorage() {
            visit(
                [](auto& val) {
                    using T = typename std::decay<decltype(val)>::type;
                    val.~T();
                }
            );
        }

        VariantStorage& operator=(VariantStorage const& other) 
            noexcept(all_noexcept_copy_constructible<Ts...>::value)
        {
            this->~VariantStorage();
            other.visit(
                [this](auto const& val) {
                    using T = typename std::decay<decltype(val)>::type;
                    new (static_cast<void*>(get_storage())) T { val };
                    type_index_ = type_index_of<0, T, Ts...>::value;
                }
            );

            return *this;
        }

        VariantStorage& operator=(VariantStorage&& other) 
            noexcept(all_noexcept_move_constructible<Ts...>::value)
        {
            this->~VariantStorage();
            std::move(other).visit(
                [this](auto&& val) {
                    using T = typename std::decay<decltype(val)>::type;
                    new (static_cast<void*>(get_storage())) T { 
                        std::move(val) 
                    };
                    type_index_ = type_index_of<0, T, Ts...>::value;
                }
            );

            return *this;
        }

        template<
            typename U,
            typename std::enable_if<
                !std::is_same<
                    typename std::decay<U>::type,
                    VariantStorage>::value>::type* = nullptr>
        VariantStorage& operator=(U&& val)
            noexcept(
                noexcept(typename std::decay<U>::type { std::declval<U>() })
            )
        {
            this->~VariantStorage();
            type_index_ = 
                type_index_of<0, typename std::decay<U>::type, Ts...>::value;
            new (static_cast<void*>(get_storage())) typename std::decay<U>::type { 
                std::forward<U>(val) 
            };

            return *this;
        }

        template<typename T>
        auto is_alternative() const -> bool {
            return type_index_of<0, T, Ts...>::value == type_index_;
        }

        template<typename T>
        auto get() & -> T& {
            if (!is_alternative<T>()) {
                throw IncorrectAlternativeError { };
            }

            return *reinterpret_cast<T*>(storage_);
        }

        template<typename T>
        auto get() const & -> T const& {
            return const_cast<VariantStorage&>(*this).get<T>();
        }

        template<typename T>
        auto get() && -> T&& {
            return std::move(get<T>());
        }

        template<size_t I>
        auto get() & -> type_at_index_t<I, Ts...>& {
            if (I != type_index_) {
                throw IncorrectAlternativeError { };
            }

            using U = type_at_index_t<I, Ts...>;

            return *reinterpret_cast<U*>(storage_);
        }

        template<size_t I>
        auto get() const & 
            -> type_at_index_t<I, Ts...> const& 
        {
            return const_cast<VariantStorage&>(*this).get<I>();
        }

        template<size_t I>
        auto get() && -> type_at_index_t<I, Ts...>&& {
            return std::move(get<I>());
        }

        template<typename F>
        decltype(auto) visit(F&& visitor) const & {
            using R = std::result_of_t<F(first_type_t<Ts...> const&)>;
            using Fr = std::add_rvalue_reference_t<F>;
            using Fn = R (*)(Fr, decltype(get_storage()));
            Fn paths[sizeof...(Ts)] = {
                apply_visitor<Ts const, Fr, decltype(get_storage())>...
            };

            return (paths[type_index_])(
                std::forward<F>(visitor), 
                get_storage());
        }

        template<typename F>
        decltype(auto) visit(F&& visitor) & {
            using R = std::result_of_t<F(first_type_t<Ts...>&)>;
            using Fr = std::add_rvalue_reference_t<F>;
            using Fn = R (*)(Fr, decltype(get_storage()));
            Fn paths[sizeof...(Ts)] = {
                apply_visitor<Ts, Fr, decltype(get_storage())>...
            };

            return (paths[type_index_])(
                std::forward<F>(visitor), 
                get_storage());
        }

        template<typename F>
        decltype(auto) visit(F&& visitor) && {
            using S = decltype(std::move(*this).get_storage());
            using R = std::result_of_t<F(first_type_t<Ts...>&&)>;
            using Fr = std::add_rvalue_reference_t<F>;
            using Fn = auto (*)(Fr, S) -> R;
            Fn paths[sizeof...(Ts)] = {
                apply_move_visitor<Ts, Fr, S>...
            };

            return (paths[type_index_])(
                std::forward<F>(visitor), 
                std::move(*this).get_storage());
        }

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

    template<typename... Ts>
    struct Variant 
        : std::conditional<all_copy_constructible<Ts...>::value, 
                           Copyable, 
                           NoCopyable>::type
    {
        static_assert(0 < sizeof...(Ts),
            "VariantStorage must hold at least one type");

        static_assert(all_move_constructible<Ts...>::value,
            "VariantStorage types must all be moveable");

        template<
            typename U,
            typename std::enable_if<
                !std::is_same<
                    typename std::decay<U>::type,
                    Variant>::value>::type* = nullptr>
        Variant(U&& val)
            noexcept(
                noexcept(typename std::decay<U>::type { std::declval<U>() })
            )
        :
            inner_ {std::forward<U>(val) }
        { }

        template<
            typename U,
            typename std::enable_if<
                !std::is_same<
                    typename std::decay<U>::type,
                    Variant>::value>::type* = nullptr>
        Variant& operator=(U&& val)
            noexcept(
                noexcept(typename std::decay<U>::type { std::declval<U>() })
            )
        {
            inner_ = val;
            return *this;
        }

        template<typename U>
        auto is_alternative() const {
            return inner_.template is_alternative<U>();
        }

        template<typename F>
        auto visit(F&& visitor) & 
            -> decltype(std::declval<VariantStorage<Ts...>&>().visit(std::forward<F>(visitor)))
        {
            return inner_.visit(std::forward<F>(visitor));
        }

        template<typename F>
        auto visit(F&& visitor) const & 
            -> decltype(std::declval<VariantStorage<Ts...> const&>().visit(std::forward<F>(visitor)))
        {
            return inner_.visit(std::forward<F>(visitor));
        }

        template<typename F>
        auto visit(F&& visitor) && 
            -> decltype(std::declval<VariantStorage<Ts...>>().visit(std::forward<F>(visitor)))
        {
            return std::move(inner_).visit(std::forward<F>(visitor));
        }

        template<typename T>
        auto get() & -> T& {
            return inner_.template get<T>();
        }

        template<typename T>
        auto get() const & -> T const& {
            return inner_.template get<T>();
        }

        template<typename T>
        auto get() && -> T&& {
            return std::move(inner_).template get<T>();
        }

        template<size_t I>
        decltype(auto) get() & {
            return inner_.template get<I>();
        }

        template<size_t I>
        decltype(auto) get() const & {
            return inner_.template get<I>();
        }

        template<size_t I>
        decltype(auto) get() && {
            return std::move(inner_).template get<I>();
        }

    private:
        VariantStorage<Ts...> inner_;
    };

    namespace traits {
        template<typename T>
        struct is_variant : std::false_type { };

        template<typename... Ts>
        struct is_variant<Variant<Ts...>> : std::true_type { };

        template<typename... Ts>
        struct is_variant<Variant<Ts...> const> : std::true_type { };

        template<typename... Ts>
        struct is_variant<Variant<Ts...>&> : std::true_type { };

        template<typename... Ts>
        struct is_variant<Variant<Ts...> const&> : std::true_type { };

        template<typename T>
        constexpr bool is_variant_v = is_variant<T>::value;
    }

    template<typename T, typename... Ts>
    auto is_alternative(Variant<Ts...> const& v) -> bool {
        return v.template is_alternative<T>();
    }

    template<
        typename F, 
        typename V,
        typename std::enable_if<traits::is_variant_v<V>>::type* = nullptr>
    decltype(auto) visit(F&& visitor, V&& var) {
        return std::forward<V>(var).visit(std::forward<F>(visitor));
    }

    template<
        typename T, 
        typename V,
        typename std::enable_if<traits::is_variant_v<V>>::type* = nullptr>
    decltype(auto) get(V&& var) {
        return std::forward<V>(var).template get<T>();
    }

    template<
        size_t I,
        typename V,
        typename std::enable_if<traits::is_variant_v<V>>::type* = nullptr>
    decltype(auto) get(V&& val) {
        return std::forward<V>(val).template get<I>();
    }
}
#endif //VARIANT_VARIANT_HPP_INCLUDED
