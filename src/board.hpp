#pragma once
#include "bitboard.hpp"
#include <iostream>
#include <string>

// Piece constants for array indexing
constexpr int PAWN = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK = 3;
constexpr int QUEEN = 4;
constexpr int KING = 5;

// Castling rights bitmasks
constexpr int CASTLE_WK = 1; // White King-side (0001)
constexpr int CASTLE_WQ = 2; // White Queen-side (0010)
constexpr int CASTLE_BK = 4; // Black King-side (0100)
constexpr int CASTLE_BQ = 8; // Black Queen-side (1000)

struct Board {
    Bitboard pieces[2][6];
    Bitboard occupancies[3];
    Color side_to_move;

    // Game State Variables
    int castling_rights;
    int en_passant_square;

    void reset() {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 6; ++j) {
                pieces[i][j] = 0ULL;
            }
        }
        for (int i = 0; i < 3; ++i) {
            occupancies[i] = 0ULL;
        }
        side_to_move = Color::White;
        castling_rights = 0;
        en_passant_square = -1;
    }

    void set_starting_position() {
        reset();

        // White Pieces
        pieces[static_cast<int>(Color::White)][PAWN] = 0x000000000000FF00ULL;
        pieces[static_cast<int>(Color::White)][KNIGHT] = 0x0000000000000042ULL;
        pieces[static_cast<int>(Color::White)][BISHOP] = 0x0000000000000024ULL;
        pieces[static_cast<int>(Color::White)][ROOK] = 0x0000000000000081ULL;
        pieces[static_cast<int>(Color::White)][QUEEN] = 0x0000000000000008ULL;
        pieces[static_cast<int>(Color::White)][KING] = 0x0000000000000010ULL;

        // Black Pieces
        pieces[static_cast<int>(Color::Black)][PAWN] = 0x00FF000000000000ULL;
        pieces[static_cast<int>(Color::Black)][KNIGHT] = 0x4200000000000000ULL;
        pieces[static_cast<int>(Color::Black)][BISHOP] = 0x2400000000000000ULL;
        pieces[static_cast<int>(Color::Black)][ROOK] = 0x8100000000000000ULL;
        pieces[static_cast<int>(Color::Black)][QUEEN] = 0x0800000000000000ULL;
        pieces[static_cast<int>(Color::Black)][KING] = 0x1000000000000000ULL;

        // Occupancies
        for (int i = 0; i < 6; ++i) {
            occupancies[static_cast<int>(Color::White)] |= pieces[static_cast<int>(Color::White)][i];
            occupancies[static_cast<int>(Color::Black)] |= pieces[static_cast<int>(Color::Black)][i];
        }
        occupancies[static_cast<int>(Color::Both)] = occupancies[static_cast<int>(Color::White)] | occupancies[static_cast<int>(Color::Black)];
        side_to_move = Color::White;

        // In the starting position, both sides can castle both ways!
        castling_rights = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
        en_passant_square = -1; // No en passant available on turn 1
    }

    // --- NEW: Fast legality check helper ---
    bool is_square_attacked(int square, Color attacker_color) const {
        int attacker = static_cast<int>(attacker_color);
        Bitboard square_bb = 1ULL << square;

        // --- 1. PAWN ATTACKS ---
        Bitboard pawns = pieces[attacker][PAWN];
        Bitboard pawn_attacks = 0;
        if (attacker_color == Color::White) {
            // White pawns attack UP-LEFT (<< 7) and UP-RIGHT (<< 9)
            pawn_attacks |= (pawns << 7) & 0x7F7F7F7F7F7F7F7FULL; // Mask out H-file wrap
            pawn_attacks |= (pawns << 9) & 0xFEFEFEFEFEFEFEFEULL; // Mask out A-file wrap
        } else {
            // Black pawns attack DOWN-RIGHT (>> 7) and DOWN-LEFT (>> 9)
            pawn_attacks |= (pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL; // Mask out A-file wrap
            pawn_attacks |= (pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL; // Mask out H-file wrap
        }
        if (pawn_attacks & square_bb) return true;

        // --- 2. KNIGHT ATTACKS ---
        // Shift the entire knight bitboard into all possible attack squares
        Bitboard knights = pieces[attacker][KNIGHT];
        if (knights) {
            Bitboard l1 = (knights >> 1) & 0x7F7F7F7F7F7F7F7FULL;
            Bitboard l2 = (knights >> 2) & 0x3F3F3F3F3F3F3F3FULL;
            Bitboard r1 = (knights << 1) & 0xFEFEFEFEFEFEFEFEULL;
            Bitboard r2 = (knights << 2) & 0xFCFCFCFCFCFCFCFCULL;
            Bitboard h1 = l1 | r1;
            Bitboard h2 = l2 | r2;
            Bitboard knight_attacks = (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
            if (knight_attacks & square_bb) return true;
        }

        // --- 3. KING ATTACKS ---
        // Helpful to detect illegal positions where kings touch
        Bitboard king = pieces[attacker][KING];
        if (king) {
            Bitboard king_adj = ((king << 1) & 0xFEFEFEFEFEFEFEFEULL) | ((king >> 1) & 0x7F7F7F7F7F7F7F7FULL);
            Bitboard king_attacks = king_adj | king;
            king_attacks |= (king_attacks << 8) | (king_attacks >> 8);
            king_attacks &= ~king; // Remove the king's own square
            if (king_attacks & square_bb) return true;
        }

        // --- 4. SLIDER ATTACKS (Rooks, Bishops, Queens) ---
        // Cast rays outward from the target square and see if we hit an enemy slider
        Bitboard occ = occupancies[static_cast<int>(Color::Both)];
        Bitboard rq = pieces[attacker][ROOK] | pieces[attacker][QUEEN];
        Bitboard bq = pieces[attacker][BISHOP] | pieces[attacker][QUEEN];

        // Directions: N, S, E, W, NE, NW, SE, SW
        int dirs[8] = {8, -8, 1, -1, 9, 7, -7, -9};
        
        for (int i = 0; i < 8; ++i) {
            int dir = dirs[i];
            bool is_diagonal = (i >= 4);
            Bitboard attackers = is_diagonal ? bq : rq;
            
            if (!attackers) continue; // Skip if no attacking pieces of this type exist

            int temp_sq = square;
            while (true) {
                // Check if we are on the edge BEFORE moving in that direction
                if ((dir == 1 || dir == 9 || dir == -7) && (temp_sq % 8 == 7)) break; // Right edge
                if ((dir == -1 || dir == 7 || dir == -9) && (temp_sq % 8 == 0)) break; // Left edge
                if ((dir == 8 || dir == 9 || dir == 7) && (temp_sq / 8 == 7)) break; // Top edge
                if ((dir == -8 || dir == -7 || dir == -9) && (temp_sq / 8 == 0)) break; // Bottom edge

                temp_sq += dir;
                
                Bitboard target_bb = 1ULL << temp_sq;
                if (target_bb & attackers) return true; // Found an enemy sliding piece!
                if (target_bb & occ) break;             // Blocked by any piece (friendly or enemy)
            }
        }

        return false;
    }

    void print() const {
        std::cout << "\n   +------------------------+\n";
        for (int rank = 7; rank >= 0; --rank) {
            std::cout << " " << rank + 1 << " |";
            for (int file = 0; file < 8; ++file) {
                int square = rank * 8 + file;
                char piece_char = '.';

                Bitboard mask = 1ULL << square;

                if (pieces[static_cast<int>(Color::White)][PAWN] & mask) piece_char = 'P';
                else if (pieces[static_cast<int>(Color::White)][KNIGHT] & mask) piece_char = 'N';
                else if (pieces[static_cast<int>(Color::White)][BISHOP] & mask) piece_char = 'B';
                else if (pieces[static_cast<int>(Color::White)][ROOK] & mask) piece_char = 'R';
                else if (pieces[static_cast<int>(Color::White)][QUEEN] & mask) piece_char = 'Q';
                else if (pieces[static_cast<int>(Color::White)][KING] & mask) piece_char = 'K';

                else if (pieces[static_cast<int>(Color::Black)][PAWN] & mask) piece_char = 'p';
                else if (pieces[static_cast<int>(Color::Black)][KNIGHT] & mask) piece_char = 'n';
                else if (pieces[static_cast<int>(Color::Black)][BISHOP] & mask) piece_char = 'b';
                else if (pieces[static_cast<int>(Color::Black)][ROOK] & mask) piece_char = 'r';
                else if (pieces[static_cast<int>(Color::Black)][QUEEN] & mask) piece_char = 'q';
                else if (pieces[static_cast<int>(Color::Black)][KING] & mask) piece_char = 'k';

                std::cout << "  " << piece_char;
            }
            std::cout << " |\n";
        }
        std::cout << "   +------------------------+\n";
        std::cout << "     a  b  c  d  e  f  g  h\n\n";
        std::cout << "Side to move: " << (side_to_move == Color::White ? "White" : "Black") << "\n";

        // Print out the Game State
        std::cout << "Castling Rights: ";
        std::cout << ((castling_rights & CASTLE_WK) ? "K" : "-");
        std::cout << ((castling_rights & CASTLE_WQ) ? "Q" : "-");
        std::cout << ((castling_rights & CASTLE_BK) ? "k" : "-");
        std::cout << ((castling_rights & CASTLE_BQ) ? "q" : "-") << "\n";

        std::cout << "En Passant: ";
        if (en_passant_square != -1) {
            char file = 'a' + (en_passant_square % 8);
            char rank = '1' + (en_passant_square / 8);
            std::cout << file << rank << "\n";
        } else {
            std::cout << "None\n";
        }
    }
};
