#pragma once
#include "types.hpp"
#include <random>

namespace Zobrist {
    inline U64 piece_keys[12][64];
    inline U64 side_key;
    inline U64 ep_keys[64];
    inline U64 castle_keys[16];

    inline void init() {
        std::mt19937_64 rng(123456789); // Fixed seed for reproducible hashes
        
        for (int p = 0; p < 12; ++p) {
            for (int sq = 0; sq < 64; ++sq) {
                piece_keys[p][sq] = rng();
            }
        }
        side_key = rng();
        for (int i = 0; i < 64; ++i) ep_keys[i] = rng();
        for (int i = 0; i < 16; ++i) castle_keys[i] = rng();
    }
}
