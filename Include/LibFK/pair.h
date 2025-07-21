#pragma once

namespace FK {

template<typename T1, typename T2>
struct Pair {
    T1 first;
    T2 second;

    constexpr Pair() noexcept = default;
    constexpr Pair(const T1& a, const T2& b) noexcept
        : first(a)
        , second(b)
    {
    }

    constexpr Pair(T1&& a, T2&& b) noexcept
        : first(static_cast<T1&&>(a))
        , second(static_cast<T2&&>(b))
    {
    }

    template<typename U1, typename U2>
    constexpr Pair(Pair<U1, U2> const& other) noexcept
        : first(other.first)
        , second(other.second)
    {
    }

    Pair& operator=(Pair const& other) noexcept
    {
        if (this != &other) {
            first = other.first;
            second = other.second;
        }
        return *this;
    }

    Pair& operator=(Pair&& other) noexcept
    {
        if (this != &other) {
            first = static_cast<T1&&>(other.first);
            second = static_cast<T2&&>(other.second);
        }
        return *this;
    }
};

template<typename T1, typename T2>
constexpr bool operator==(Pair<T1, T2> const& lhs, Pair<T1, T2> const& rhs) noexcept
{
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

template<typename T1, typename T2>
constexpr bool operator!=(Pair<T1, T2> const& lhs, Pair<T1, T2> const& rhs) noexcept
{
    return !(lhs == rhs);
}

};
