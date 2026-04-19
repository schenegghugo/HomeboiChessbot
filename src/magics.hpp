#pragma once
#include "types.hpp"
#include "board.hpp"

// Lookup tables for non-sliding pieces
extern U64 KnightAttacks[64];
extern U64 KingAttacks[64];

// --- INITIALIZATION ---
// Call this ONCE inside main() before doing anything else!
inline void init_magics() {
    for (int sq = 0; sq < 64; ++sq) {
        U64 piece = (1ULL << sq);
        
        // 1. Calculate Knight Attacks
        U64 l1 = (piece >> 1) & 0x7f7f7f7f7f7f7f7fULL;
        U64 l2 = (piece >> 2) & 0x3f3f3f3f3f3f3f3fULL;
        U64 r1 = (piece << 1) & 0xfefefefefefefefeULL;
        U64 r2 = (piece << 2) & 0xfcfcfcfcfcfcfcfcULL;
        
        U64 h1 = l1 | r1;
        U64 h2 = l2 | r2;
        
        KnightAttacks[sq] = (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);

        // 2. Calculate King Attacks
        U64 k = piece;
        U64 king_moves = (k << 8) | (k >> 8); // North, South
        k |= king_moves;                      // Add to original
        king_moves |= (k << 1) & 0xfefefefefefefefeULL; // East
        king_moves |= (k >> 1) & 0x7f7f7f7f7f7f7f7fULL; // West
        
        KingAttacks[sq] = king_moves & ~(1ULL << sq); // Remove original square
    }
}

// --- SLIDING PIECE ATTACKS ---
// (Simplified on-the-fly Raycasting. In a true Magic Bitboard implementation, 
// this is replaced by a lookup: return RookMagicAttacks[sq][hash(blockers)])

inline U64 get_bishop_attacks(int sq, U64 blockers) {
    U64 attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1, j = f + 1; i <= 7 && j <= 7; ++i, ++j) { attacks |= (1ULL << (i * 8 + j)); if (blockers & (1ULL << (i * 8 + j))) break; } // NE
    for (int i = r + 1, j = f - 1; i <= 7 && j >= 0; ++i, --j) { attacks |= (1ULL << (i * 8 + j)); if (blockers & (1ULL << (i * 8 + j))) break; } // NW
    for (int i = r - 1, j = f + 1; i >= 0 && j <= 7; --i, ++j) { attacks |= (1ULL << (i * 8 + j)); if (blockers & (1ULL << (i * 8 + j))) break; } // SE
    for (int i = r - 1, j = f - 1; i >= 0 && j >= 0; --i, --j) { attacks |= (1ULL << (i * 8 + j)); if (blockers & (1ULL << (i * 8 + j))) break; } // SW
    return attacks;
}

inline U64 get_rook_attacks(int sq, U64 blockers) {
    U64 attacks = 0;
    int r = sq / 8, f = sq % 8;
    for (int i = r + 1; i <= 7; ++i) { attacks |= (1ULL << (i * 8 + f)); if (blockers & (1ULL << (i * 8 + f))) break; } // North
    for (int i = r - 1; i >= 0; --i) { attacks |= (1ULL << (i * 8 + f)); if (blockers & (1ULL << (i * 8 + f))) break; } // South
    for (int j = f + 1; j <= 7; ++j) { attacks |= (1ULL << (r * 8 + j)); if (blockers & (1ULL << (r * 8 + j))) break; } // East
    for (int j = f - 1; j >= 0; --j) { attacks |= (1ULL << (r * 8 + j)); if (blockers & (1ULL << (r * 8 + j))) break; } // West
    return attacks;
}

inline U64 get_queen_attacks(int sq, U64 blockers) {
    return get_rook_attacks(sq, blockers) | get_bishop_attacks(sq, blockers);
}
