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

#define ENSURE_THROWS(expr) \
do { \
    bool expression_threw = false; \
    try { \
        (expr); \
    } \
    catch (...) { expression_threw = true; } \
    if (!expression_threw) { \
        throw std::logic_error { \
            __FILE__ ", " TO_STR(__LINE__) \
                ": Expression expected to throw - " TO_STR(expr) \
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

auto operator<<(std::ostream& os, A const&) -> std::ostream& {
    return os << "type A";
}

static_assert(!std::is_copy_constructible<A>::value,
    "Type `A` shouldn't be copy constructible");

auto type_index_tests() {

    using MyVariant = variant::Variant<int, A, std::string>;
    MyVariant v { 42 };
    //MyVariant u = v; // Shouldn't compile!
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
    static_assert(variant::all_copy_constructible<int, float, std::string>::value,
        "(int, float, string) should all be copy constructible");
    using MyVariant = variant::Variant<int, float, std::string>;
    MyVariant a { 42 };
    MyVariant b = a;
    ENSURE(variant::is_alternative<int>(b));
}

auto copy_assign_tests() {

    using MyVariant = variant::Variant<int, float, std::string>;
    MyVariant a { 42 };
    MyVariant b { std::string { "Hello, World!" } };

    b = a;

    ENSURE(variant::is_alternative<int>(b));
}

auto visit_tests() {
    using MyVariant = variant::Variant<int, A, std::string>;

    bool visited = false;
    variant::visit(
        [&visited](auto&&) {
            visited = true;
        },
        MyVariant { 42 }
    );

    ENSURE(visited);
}

auto access_tests() {
    using MyVariant = variant::Variant<int, A, std::string>;
    ENSURE(variant::get<int>(MyVariant { 42 }) == 42);
    ENSURE_THROWS(variant::get<A>(MyVariant { 42 }));
    ENSURE(std::is_rvalue_reference<decltype(variant::get<A>(std::declval<MyVariant>()))>::value);
}

auto noexcept_tests() {
    using MyVariant = variant::Variant<int, A, std::string>;
    using MyOtherVariant = variant::Variant<int, float>;

    ENSURE(std::is_nothrow_move_constructible<MyVariant>::value);
    ENSURE(!std::is_nothrow_copy_constructible<MyVariant>::value);

    ENSURE(std::is_nothrow_move_constructible<MyOtherVariant>::value);
    ENSURE(std::is_nothrow_copy_constructible<MyOtherVariant>::value);
}

using TestFunc = void (*)();

template<size_t N>
auto run_tests(TestFunc (&fn)[N]) -> bool {

    bool all_passed = true;
    for(auto&& f : fn) {
        try {
            f();
        }
        catch(std::exception const& e) {
            all_passed = false;
            std::cerr << e.what() << "\n";
        }
    }

    return all_passed;
}

auto main(int, char const**) -> int {

    TestFunc tests[] = {
        type_index_tests,
        is_alternative_tests,
        destructor_called_tests,
        noexcept_constructor_tests,
        copy_construct_tests,
        visit_tests,
        access_tests,
        noexcept_tests,
        copy_assign_tests
    };

    if (!run_tests(tests)) {
        return -1;
    }

    return 0;
}
