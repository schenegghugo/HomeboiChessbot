#pragma once
#include "attacks.hpp"
#include <vector>
#include <bit>
#include <iostream>

namespace Magics {

inline Bitboard random_uint64() {
    static uint64_t seed = 1070372ULL;
    seed ^= seed >> 12; seed ^= seed << 25; seed ^= seed >> 27;
    return seed * 2685821657736338717ULL;
}

inline Bitboard random_magic() {
    return random_uint64() & random_uint64() & random_uint64();
}

// ROOK TABLES (Max 12 bits -> 4096 size)
inline int ROOK_RELEVANT_BITS[64];
inline Bitboard ROOK_MASKS[64];
inline Bitboard ROOK_ATTACKS[64][4096]; 
inline Bitboard ROOK_MAGICS[64];

// BISHOP TABLES (Max 9 bits -> 512 size)
inline int BISHOP_RELEVANT_BITS[64];
inline Bitboard BISHOP_MASKS[64];
inline Bitboard BISHOP_ATTACKS[64][512]; 
inline Bitboard BISHOP_MAGICS[64];

inline void init_magic_bitboards() {
    std::cout << "Finding Rooks Magics...\n";
    for (int sq = 0; sq < 64; ++sq) {
        ROOK_MASKS[sq] = Attacks::mask_rook_attacks(static_cast<Square>(sq));
        ROOK_RELEVANT_BITS[sq] = std::popcount(ROOK_MASKS[sq]);
        int occupancy_variations = 1 << ROOK_RELEVANT_BITS[sq];
        
        std::vector<Bitboard> occupancies(occupancy_variations);
        std::vector<Bitboard> true_attacks(occupancy_variations);
        for (int index = 0; index < occupancy_variations; ++index) {
            occupancies[index] = Attacks::set_occupancy(index, ROOK_RELEVANT_BITS[sq], ROOK_MASKS[sq]);
            true_attacks[index] = Attacks::rook_attacks_on_the_fly(static_cast<Square>(sq), occupancies[index]);
        }

        bool found = false;
        while (!found) {
            Bitboard magic = random_magic();
            if (std::popcount((ROOK_MASKS[sq] * magic) & 0xFF00000000000000ULL) < 6) continue;

            std::vector<Bitboard> used(4096, 0ULL); 
            bool fail = false;
            for (int index = 0; index < occupancy_variations; ++index) {
                int magic_index = (occupancies[index] * magic) >> (64 - 12);
                if (used[magic_index] == 0ULL) used[magic_index] = true_attacks[index];
                else if (used[magic_index] != true_attacks[index]) { fail = true; break; }
            }
            if (!fail) {
                ROOK_MAGICS[sq] = magic; 
                found = true;
                for(int i = 0; i < 4096; i++) ROOK_ATTACKS[sq][i] = used[i];
            }
        }
    }

    std::cout << "Finding Bishops Magics...\n";
    for (int sq = 0; sq < 64; ++sq) {
        BISHOP_MASKS[sq] = Attacks::mask_bishop_attacks(static_cast<Square>(sq));
        BISHOP_RELEVANT_BITS[sq] = std::popcount(BISHOP_MASKS[sq]);
        int occupancy_variations = 1 << BISHOP_RELEVANT_BITS[sq];
        
        std::vector<Bitboard> occupancies(occupancy_variations);
        std::vector<Bitboard> true_attacks(occupancy_variations);
        for (int index = 0; index < occupancy_variations; ++index) {
            occupancies[index] = Attacks::set_occupancy(index, BISHOP_RELEVANT_BITS[sq], BISHOP_MASKS[sq]);
            true_attacks[index] = Attacks::bishop_attacks_on_the_fly(static_cast<Square>(sq), occupancies[index]);
        }

        bool found = false;
        while (!found) {
            Bitboard magic = random_magic();
            if (std::popcount((BISHOP_MASKS[sq] * magic) & 0xFF00000000000000ULL) < 6) continue;

            std::vector<Bitboard> used(512, 0ULL); // Bishop only needs 9 bits (512 slots)
            bool fail = false;
            for (int index = 0; index < occupancy_variations; ++index) {
                int magic_index = (occupancies[index] * magic) >> (64 - 9);
                if (used[magic_index] == 0ULL) used[magic_index] = true_attacks[index];
                else if (used[magic_index] != true_attacks[index]) { fail = true; break; }
            }
            if (!fail) {
                BISHOP_MAGICS[sq] = magic; 
                found = true;
                for(int i = 0; i < 512; i++) BISHOP_ATTACKS[sq][i] = used[i];
            }
        }
    }
}

// --- FAST GETTERS ---

inline Bitboard get_rook_attacks(Square sq, Bitboard blockers) noexcept {
    blockers &= ROOK_MASKS[static_cast<int>(sq)];
    int magic_index = (blockers * ROOK_MAGICS[static_cast<int>(sq)]) >> (64 - 12);
    return ROOK_ATTACKS[static_cast<int>(sq)][magic_index];
}

inline Bitboard get_bishop_attacks(Square sq, Bitboard blockers) noexcept {
    blockers &= BISHOP_MASKS[static_cast<int>(sq)];
    int magic_index = (blockers * BISHOP_MAGICS[static_cast<int>(sq)]) >> (64 - 9);
    return BISHOP_ATTACKS[static_cast<int>(sq)][magic_index];
}

inline Bitboard get_queen_attacks(Square sq, Bitboard blockers) noexcept {
    return get_rook_attacks(sq, blockers) | get_bishop_attacks(sq, blockers);
}

} // namespace Magics
