#ifndef VARIANT_VARIANT_HPP_INCLUDED
#define VARIANT_VARIANT_HPP_INCLUDED

#include <algorithm>
#include <cassert>

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

    template<typename T, typename F, typename S, typename... Ts>
    auto apply_visitor(F&& f, S storage, Ts&&... args) {
        return 
            std::forward<F>(f)(*reinterpret_cast<T*>(storage), std::forward<Ts>(args)...);
    }

    template<typename... Ts>
    struct Variant {

        template<typename U>
        explicit Variant(U&& val) :
            type_index_ { 
                type_index_of<0, typename std::decay<U>::type, Ts...>::value 
            } 
        {
            new (storage_) typename std::decay<U>::type { std::forward<U>(val) };
        }

        Variant(Variant const&) = delete;

        ~Variant() {
            //  We need *visitors* for this. Can we implement this
            //  using function pointers for the different types...
            //  ```
            //  auto f = [call_visitor<Ts>...];
            //  f[type_index_](visitor_imp{}, storage_);
            //  ```
            auto destructor = [](auto& val) {
                using T = typename std::decay<decltype(val)>::type;
                val.~T();
            };

            using Fn = void (*)(decltype(destructor)&&, decltype(&storage_[0]));

            Fn paths[sizeof...(Ts)] = {
                apply_visitor<Ts, decltype(destructor)>...
            };

            (paths[type_index_])(std::move(destructor), &storage_[0]);
        }

        Variant& operator=(Variant const&) = delete;

        template<typename T, typename... Ts>
        friend auto is_alternative(Variant<Ts...> const&) -> bool;

    private:
        using Storage = 
            typename std::aligned_storage<max_size<Ts...>(),
                                           max_align<Ts...>()>::type;
        size_t type_index_;
        Storage storage_[1];
    };

    template<typename T, typename... Ts>
    auto is_alternative(Variant<Ts...> const& v) -> bool {
        return type_index_of<0, T, Ts...>::value == v.type_index_;
    }
}
#endif //VARIANT_VARIANT_HPP_INCLUDED
