#pragma once

#include "panic.h"
#include <optional>

namespace z {
template <class T> class Result {
    std::optional<T> val;

public:
    Result() = default;
    explicit Result(T val) : val(std::move(val)) {}

    [[nodiscard]] bool is_valid() const { return val.has_value(); }
    const T& get() {
        if (val.has_value()) {
            return val.value();
        }

        panic("Result does not hold value");
    }
    T take() {
        if (val.has_value()) {
            T tmp = std::move(val.value());
            val.reset();
            return tmp;
        }

        panic("Result does not hold value");
    }
};
} // namespace z
