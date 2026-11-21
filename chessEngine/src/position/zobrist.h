#pragma once
#include <cstdint>
#include "bitboard/types.h"

namespace Zobrist {

struct Keys {
    Key psq[PIECE_NB][SQUARE_NB];
    Key side;
    Key castling[16];
    Key epFile[8];
};

constexpr Key splitmix64(uint64_t& state) {
    uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

constexpr Keys make_keys() {
    Keys k{};
    uint64_t state = 0x48796472614368ULL;
    for (auto& piece : k.psq)
        for (Key& sq : piece)
            sq = splitmix64(state);
    k.side = splitmix64(state);
    for (Key& c : k.castling)
        c = splitmix64(state);
    for (Key& f : k.epFile)
        f = splitmix64(state);
    return k;
}

inline constexpr Keys keys = make_keys();

}
