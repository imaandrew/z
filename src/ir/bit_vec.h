#pragma once

#include "core/types.h"
#include <algorithm>
#include <bit>
#include <vector>
namespace z {
class BitVector {
    static constexpr usize BITWORD_SIZE = 64;
    std::vector<u64> words;
    usize active_bits;

    static u32 num_bit_words(u32 bits) {
        return (bits + BITWORD_SIZE - 1) / BITWORD_SIZE;
    }

    static u32 bitword_pos(u32 b) { return b % BITWORD_SIZE; }

public:
    explicit BitVector(u32 bits)
        : words(num_bit_words(bits)), active_bits(bits) {};

    [[nodiscard]] usize num_words() const { return words.size(); }

    void resize(usize s) {
        words.resize(num_bit_words(s));
        active_bits = s;
    }

    [[nodiscard]] usize size() const { return active_bits; }

    bool operator==(const BitVector& rhs) const = default;

    BitVector& operator|=(const BitVector& rhs) {
        if (size() < rhs.size()) {
            resize(rhs.size());
        }

        for (usize i = 0; i < rhs.num_words(); i++) {
            words[i] |= rhs.words[i];
        }

        return *this;
    }

    BitVector& operator^=(const BitVector& rhs) {
        if (size() < rhs.size()) {
            resize(rhs.size());
        }

        for (usize i = 0; i < rhs.num_words(); i++) {
            words[i] ^= rhs.words[i];
        }

        return *this;
    }

    BitVector& operator&=(const BitVector& rhs) {
        usize i = 0;
        for (; i < std::min(num_words(), rhs.num_words()); i++) {
            words[i] &= rhs.words[i];
        }

        for (; i < num_words(); i++) {
            words[i] = 0;
        }

        return *this;
    }

    void andnot_assign(const BitVector& rhs) noexcept {
        for (usize i = 0; i < std::min(num_words(), rhs.num_words()); ++i)
            words[i] &= ~rhs.words[i];
    }

    void set(u32 b) { words[b / BITWORD_SIZE] |= 1ULL << bitword_pos(b); }

    void unset(u32 b) { words[b / BITWORD_SIZE] &= ~(1ULL << bitword_pos(b)); }

    [[nodiscard]] bool test(u32 b) const {
        return words[b / BITWORD_SIZE] & (1ULL << bitword_pos(b));
    }

    template <typename F> void for_each_set(F&& f) const {
        for (usize i = 0; i < num_words(); i++) {
            u64 word = words[i];
            if (i == num_words() - 1 && active_bits % BITWORD_SIZE != 0)
                word &= (1ULL << (active_bits % BITWORD_SIZE)) - 1;

            while (word) {
                const int bit = std::countr_zero(word);
                f(static_cast<u32>((i * BITWORD_SIZE) + bit));
                word &= word - 1;
            }
        }
    }

    template <typename F> void for_each_word(F&& f) const {
        for (usize i = 0; i < num_words(); i++) {
            u64 word = words[i];
            if (i == num_words() - 1 && active_bits % BITWORD_SIZE != 0)
                word &= (1ULL << (active_bits % BITWORD_SIZE)) - 1;

            std::forward<F>(f)(word);
        }
    }

    [[nodiscard]] u32 count() const {
        auto c = 0;
        for_each_word([&c](auto word) { c += std::popcount(word); });
        return c;
    }
};
} // namespace z
