#pragma once

template<class T>
inline void hash_combine(std::size_t& seed, const T& value) noexcept
{
    std::size_t h = std::hash<T>{}(value);

    // 64-bit golden ratio constant (works fine also if size_t is 32)
    constexpr std::size_t k = 0x9e3779b97f4a7c15ull;

    seed ^= h + k + std::rotl(seed, 6) + (seed >> 2);
}