#pragma once
#include "bitboard.hpp"
#include <bit>

namespace Attacks {

// --- ROOK ---
inline Bitboard mask_rook_attacks(Square sq) {
    Bitboard attacks = 0ULL;
    int r = static_cast<int>(sq) / 8;
    int f = static_cast<int>(sq) % 8;
    for (int tr = r + 1; tr <= 6; tr++) set_bit(attacks, static_cast<Square>(tr * 8 + f));
    for (int tr = r - 1; tr >= 1; tr--) set_bit(attacks, static_cast<Square>(tr * 8 + f));
    for (int tf = f + 1; tf <= 6; tf++) set_bit(attacks, static_cast<Square>(r * 8 + tf));
    for (int tf = f - 1; tf >= 1; tf--) set_bit(attacks, static_cast<Square>(r * 8 + tf));
    return attacks;
}

inline Bitboard rook_attacks_on_the_fly(Square sq, Bitboard block) {
    Bitboard attacks = 0ULL;
    int r = static_cast<int>(sq) / 8;
    int f = static_cast<int>(sq) % 8;
    for (int tr = r + 1; tr <= 7; tr++) { set_bit(attacks, static_cast<Square>(tr * 8 + f)); if ((1ULL << (tr * 8 + f)) & block) break; }
    for (int tr = r - 1; tr >= 0; tr--) { set_bit(attacks, static_cast<Square>(tr * 8 + f)); if ((1ULL << (tr * 8 + f)) & block) break; }
    for (int tf = f + 1; tf <= 7; tf++) { set_bit(attacks, static_cast<Square>(r * 8 + tf)); if ((1ULL << (r * 8 + tf)) & block) break; }
    for (int tf = f - 1; tf >= 0; tf--) { set_bit(attacks, static_cast<Square>(r * 8 + tf)); if ((1ULL << (r * 8 + tf)) & block) break; }
    return attacks;
}

// --- BISHOP ---
inline Bitboard mask_bishop_attacks(Square sq) {
    Bitboard attacks = 0ULL;
    int r = static_cast<int>(sq) / 8;
    int f = static_cast<int>(sq) % 8;
    for (int tr = r + 1, tf = f + 1; tr <= 6 && tf <= 6; tr++, tf++) set_bit(attacks, static_cast<Square>(tr * 8 + tf));
    for (int tr = r - 1, tf = f + 1; tr >= 1 && tf <= 6; tr--, tf++) set_bit(attacks, static_cast<Square>(tr * 8 + tf));
    for (int tr = r + 1, tf = f - 1; tr <= 6 && tf >= 1; tr++, tf--) set_bit(attacks, static_cast<Square>(tr * 8 + tf));
    for (int tr = r - 1, tf = f - 1; tr >= 1 && tf >= 1; tr--, tf--) set_bit(attacks, static_cast<Square>(tr * 8 + tf));
    return attacks;
}

inline Bitboard bishop_attacks_on_the_fly(Square sq, Bitboard block) {
    Bitboard attacks = 0ULL;
    int r = static_cast<int>(sq) / 8;
    int f = static_cast<int>(sq) % 8;
    for (int tr = r + 1, tf = f + 1; tr <= 7 && tf <= 7; tr++, tf++) { set_bit(attacks, static_cast<Square>(tr * 8 + tf)); if ((1ULL << (tr * 8 + tf)) & block) break; }
    for (int tr = r - 1, tf = f + 1; tr >= 0 && tf <= 7; tr--, tf++) { set_bit(attacks, static_cast<Square>(tr * 8 + tf)); if ((1ULL << (tr * 8 + tf)) & block) break; }
    for (int tr = r + 1, tf = f - 1; tr <= 7 && tf >= 0; tr++, tf--) { set_bit(attacks, static_cast<Square>(tr * 8 + tf)); if ((1ULL << (tr * 8 + tf)) & block) break; }
    for (int tr = r - 1, tf = f - 1; tr >= 0 && tf >= 0; tr--, tf--) { set_bit(attacks, static_cast<Square>(tr * 8 + tf)); if ((1ULL << (tr * 8 + tf)) & block) break; }
    return attacks;
}

// Map index to specific blocker configurations
inline Bitboard set_occupancy(int index, int bits_in_mask, Bitboard attack_mask) {
    Bitboard occupancy = 0ULL;
    for (int count = 0; count < bits_in_mask; count++) {
        int square = std::countr_zero(attack_mask);
        clear_bit(attack_mask, static_cast<Square>(square));
        if (index & (1 << count)) {
            occupancy |= (1ULL << square);
        }
    }
    return occupancy;
}

// --- LEAPERS ---
// Notice how these are now properly INSIDE the namespace!
inline Bitboard KNIGHT_ATTACKS[64];
inline Bitboard KING_ATTACKS[64];

inline void init_leapers() {
    for (int sq = 0; sq < 64; ++sq) {
        int r = sq / 8;
        int f = sq % 8;

        Bitboard knight = 0ULL;
        int knight_offsets[8][2] = {{2,1}, {1,2}, {-1,2}, {-2,1}, {-2,-1}, {-1,-2}, {1,-2}, {2,-1}};
        for (auto& off : knight_offsets) {
            int tr = r + off[0], tf = f + off[1];
            if (tr >= 0 && tr <= 7 && tf >= 0 && tf <= 7) set_bit(knight, static_cast<Square>(tr * 8 + tf));
        }
        KNIGHT_ATTACKS[sq] = knight;

        Bitboard king = 0ULL;
        int king_offsets[8][2] = {{1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, {0,-1}, {1,-1}};
        for (auto& off : king_offsets) {
            int tr = r + off[0], tf = f + off[1];
            if (tr >= 0 && tr <= 7 && tf >= 0 && tf <= 7) set_bit(king, static_cast<Square>(tr * 8 + tf));
        }
        KING_ATTACKS[sq] = king;
    }
}

} // <--- The closing brace for namespace Attacks is now at the very end!
