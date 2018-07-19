#include "variant/variant.hpp"
#include <string>
#include <stdexcept>

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

    explicit A(bool* flag) :
        flag_ { flag }
    { }

    A(A&& other) :
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

auto main(int, char const**) -> int {

    using MyVariant = variant::Variant<int, A, std::string>;

    bool destructor_called = false;
    {
        MyVariant v { A { &destructor_called } };
        ENSURE(get_type_index<int>(v) == 0);
        ENSURE(get_type_index<A>(v) == 1);
        ENSURE(get_type_index<std::string>(v) == 2);
        ENSURE(variant::is_alternative<A>(v));
        ENSURE(!variant::is_alternative<std::string>(v));
    }

    ENSURE(destructor_called);

    return 0;
}
