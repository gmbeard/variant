#ifndef VARIANT_EXAMPLES_JSON_HPP_INCLUDED
#define VARIANT_EXAMPLES_JSON_HPP_INCLUDED

#include "variant/variant.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <string>

namespace json {

    struct JsonString { std::string value; };
    struct JsonNumber { double value; };
    struct JsonArray; 
    struct JsonObject;
    struct JsonNull { };

    template<typename T>
    struct JsonProxy {

        JsonProxy(T val) :
            inner { std::make_unique<T>(val) }
        { }

        JsonProxy(JsonProxy const& other) :
            inner { std::make_unique<T>(*other.inner) }
        { }

        JsonProxy(JsonProxy&&) = default;

        JsonProxy& operator=(JsonProxy const& other) {
            using std::swap;
            JsonProxy tmp { other };
            swap(tmp.inner, inner);
            return *this;
        }

        JsonProxy& operator=(JsonProxy&&) = default;

        operator T&() {
            return *inner;
        }

        operator T const&() const {
            return *inner;
        }

        T* operator->() {
            return inner.get();
        }

        T const* operator->() const {
            return inner.get();
        }

        T& operator*() {
            return *inner;
        }

        T const& operator*() const {
            return *inner;
        }

        std::unique_ptr<T> inner;
    };

    using JsonArrayProxy = JsonProxy<JsonArray>;
    using JsonObjectProxy = JsonProxy<JsonObject>;

    auto operator<<(std::ostream&, JsonString const&) -> std::ostream&;
    auto operator<<(std::ostream&, JsonNumber const&) -> std::ostream&;
    auto operator<<(std::ostream&, JsonArray const&) -> std::ostream&;
    auto operator<<(std::ostream&, JsonObject const&) -> std::ostream&;

    inline auto operator<<(std::ostream& os, JsonNull const&) -> std::ostream& {
        return os << "null";
    }

    template<typename T>
    inline auto operator<<(std::ostream& os, JsonProxy<T> const& obj) 
        -> std::ostream&
    {
        return os << *obj;
    }

    using JsonValue = 
        variant::Variant<JsonString,
                         JsonNumber,
                         JsonArrayProxy,
                         JsonObjectProxy,
                         JsonNull>;

    using PropValuePair = std::pair<std::string const, JsonValue>;

    struct JsonArray {
        std::vector<JsonValue> values;
    };

    struct JsonObject {
        std::unordered_map<std::string, JsonValue> members;
    };

    struct JsonOutputVisitor {
        JsonOutputVisitor(std::ostream& os)
            : os_{ os }
        { }

        auto operator()(JsonString const& s) -> std::ostream& {
            return os_ << "\"" << s.value << "\"";
        }

        auto operator()(JsonNumber const& n) -> std::ostream& {
            return os_ << n.value;
        }

        template<typename T>
        auto operator()(JsonProxy<T> const& p) -> std::ostream& {
            return os_ << *p;
        }
        
        auto operator()(JsonNull const&) -> std::ostream& {
            return os_ << "null";
        }

    private:
        std::ostream& os_;
    };

    inline auto operator<<(std::ostream& os, JsonValue const& obj) -> std::ostream& {
        return variant::visit(JsonOutputVisitor { os }, obj);
    }

    inline auto operator<<(std::ostream& os, JsonArray const& obj) -> std::ostream& {
        os << "[";
        auto first = obj.values.begin();
        if (first != obj.values.end()) {
            os << *first;
            while (++first != obj.values.end()) {
                os << ", " << *first;
            }
        }
        return os << "]";
    }

    inline auto operator<<(std::ostream& os, JsonObject const& obj) -> std::ostream& {
        os << "{ ";

        auto first = obj.members.begin();
        if (first != obj.members.end()) {
            os << "\"" << std::get<0>(*first) << "\": "
                << std::get<1>(*first);

            while (++first != obj.members.end()) {
                os << ", \"" 
                    << std::get<0>(*first) 
                    << "\": " 
                    << std::get<1>(*first) ;
            }
        }
        return os << "}";
    }

    inline auto array(std::initializer_list<JsonValue> values) 
        -> JsonProxy<JsonArray> 
    {
        return JsonArray { values };
    }

    inline auto string(std::string const& s) -> JsonValue {
        return JsonString { s };
    }

    inline auto number(double n) -> JsonValue {
        return JsonNumber { n };
    }

    inline auto null() -> JsonValue {
        return JsonNull { };
    }

    inline auto object(std::initializer_list<PropValuePair> v) 
        -> JsonProxy<JsonObject> 
    {
        return JsonObject { v };
    }
}
#endif //VARIANT_EXAMPLES_JSON_HPP_INCLUDED
