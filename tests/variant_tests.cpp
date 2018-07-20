#include "variant/variant.hpp"
#include <string>
#include <stdexcept>
#include <type_traits>
#include <iostream>

#define TO_STR_IMPL(x) #x
#define TO_STR(x) TO_STR_IMPL(x)
#define ENSURE(cond) \
do { \
    if (!(cond)) { \
        throw std::logic_error { \
            __FILE__ ", " TO_STR(__LINE__) \
                ": Condition not met - " TO_STR(cond) \
        }; \
    } \
} \
while (false)

template<typename T, typename... Ts>
constexpr auto get_type_index(variant::Variant<Ts...> const&) -> size_t {
    return variant::type_index_of<0, T, Ts...>::value;
}

struct A {

    explicit A(bool* flag) noexcept :
        flag_ { flag }
    { }

    A(A&& other) noexcept :
        flag_ { other.flag_ }
    {
        other.flag_ = nullptr;
    }

    A(A const&) = delete;
    A& operator=(A&&) = delete;
    A& operator=(A const&) = delete; 

    ~A() {
        if (flag_) {
            *flag_ = true;
        }
    }
private:
    bool* flag_;
};

auto type_index_tests() {

    using MyVariant = variant::Variant<int, A, std::string>;
    MyVariant v { 42 };
    ENSURE(get_type_index<int>(v) == 0);
    ENSURE(get_type_index<A>(v) == 1);
    ENSURE(get_type_index<std::string>(v) == 2);
}

auto is_alternative_tests() {

    using MyVariant = variant::Variant<int, A, std::string>;
    MyVariant v { std::string { } };
    ENSURE(variant::is_alternative<std::string>(v));
    ENSURE(!variant::is_alternative<A>(v));
}

auto destructor_called_tests() {
    using MyVariant = variant::Variant<int, A, std::string>;

    bool destructor_called = false;
    {
        MyVariant v { A { &destructor_called } };
        ENSURE(variant::is_alternative<A>(v));
        ENSURE(!variant::is_alternative<std::string>(v));
    }

    ENSURE(destructor_called);
}

auto noexcept_constructor_tests() {
    using MyVariant = variant::Variant<int, A, std::string>;
    ENSURE(noexcept(std::string { }));
    ENSURE(!noexcept(std::string { std::declval<std::string const&>() }));
    ENSURE(noexcept(MyVariant {std::declval<std::string>()}));
    ENSURE(!noexcept(MyVariant { std::declval<std::string const&>()}));
}

auto copy_construct_tests() {
    using MyVariant = variant::Variant<int, float, std::string>;
    MyVariant a { 42 };
    MyVariant b = a;
    ENSURE(variant::is_alternative<int>(b));
}

auto main(int, char const**) -> int {

    try {
        type_index_tests();
        is_alternative_tests();
        destructor_called_tests();
        noexcept_constructor_tests();
        copy_construct_tests();
    }
    catch(std::exception const& e) {
        std::cerr << e.what() << "\n";
        return -1;
    }
    return 0;
}
