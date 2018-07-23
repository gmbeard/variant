#include "json.hpp"
#include <iostream>

auto main(int, char const**) -> int {

    auto obj = json::object({
        { "Foo", json::number(42.0) },
        { "Bar", json::string("Hello, World!") },
        { 
            "Baz",
            json::array({
                json::object({
                    { "A", json::number(43.0) },
                    { "B", json::string("Goodbye, World!") },
                    { "C", json::null() }
                }),
                json::number(44.0)
            })
        }
    });

    std::cout << obj << "\n";
}
