#pragma once
#include "types.hpp"

// Set a bit to 1 (Put a piece on a square)
constexpr void set_bit(Bitboard& bb, Square sq) noexcept {
    bb |= (1ULL << static_cast<int>(sq));
}

// Set a bit to 0 (Remove a piece from a square)
constexpr void clear_bit(Bitboard& bb, Square sq) noexcept {
    bb &= ~(1ULL << static_cast<int>(sq));
}

// Check if a bit is 1 (Is there a piece on this square?)
constexpr bool test_bit(Bitboard bb, Square sq) noexcept {
    return bb & (1ULL << static_cast<int>(sq));
}

// Debugging Tool: Prints a 64-bit integer as an 8x8 Chess Board
inline void print_bitboard(Bitboard bb) {
    std::cout << "\n";
    // We loop from Rank 8 down to Rank 1 so it prints correctly top-to-bottom
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << "  "; // Print Rank number
        
        for (int file = 0; file < 8; ++file) {
            // Calculate the bit index (0-63)
            int square_index = rank * 8 + file;
            
            // Test if the bit is 1 or 0
            if (test_bit(bb, static_cast<Square>(square_index))) {
                std::cout << "1 ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n\n";
    std::cout << "Bitboard Value: " << bb << "ULL\n";
}
