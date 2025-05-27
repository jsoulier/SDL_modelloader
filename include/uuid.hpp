#pragma once

#include <cstdint>

class UUID
{
public:
    constexpr UUID(int32_t value = -1)
        : value{value} {}

    constexpr int32_t get_value() const
    {
        return value;
    }

    constexpr bool operator==(const UUID other) const
    {
        return value == other.value;
    }

    constexpr operator bool() const
    {
        return value != -1;
    }

private:
    int32_t value;
};