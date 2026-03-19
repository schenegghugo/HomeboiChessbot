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

    // --- HISTORY TRACKING FOR DRAWS ---
    int half_move_clock;
    int history_ply;
    uint64_t repetition_history[2048];

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

        // Reset history
        half_move_clock = 0;
        history_ply = 0;
        for (int i = 0; i < 2048; ++i) {
            repetition_history[i] = 0ULL;
        }
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

        castling_rights = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
        en_passant_square = -1; 
    }

    bool is_square_attacked(int square, Color attacker_color) const {
        int attacker = static_cast<int>(attacker_color);
        Bitboard square_bb = 1ULL << square;

        Bitboard pawns = pieces[attacker][PAWN];
        Bitboard pawn_attacks = 0;
        if (attacker_color == Color::White) {
            pawn_attacks |= (pawns << 7) & 0x7F7F7F7F7F7F7F7FULL;
            pawn_attacks |= (pawns << 9) & 0xFEFEFEFEFEFEFEFEULL; 
        } else {
            pawn_attacks |= (pawns >> 7) & 0xFEFEFEFEFEFEFEFEULL; 
            pawn_attacks |= (pawns >> 9) & 0x7F7F7F7F7F7F7F7FULL; 
        }
        if (pawn_attacks & square_bb) return true;

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

        Bitboard king = pieces[attacker][KING];
        if (king) {
            Bitboard king_adj = ((king << 1) & 0xFEFEFEFEFEFEFEFEULL) | ((king >> 1) & 0x7F7F7F7F7F7F7F7FULL);
            Bitboard king_attacks = king_adj | king;
            king_attacks |= (king_attacks << 8) | (king_attacks >> 8);
            king_attacks &= ~king; 
            if (king_attacks & square_bb) return true;
        }

        Bitboard occ = occupancies[static_cast<int>(Color::Both)];
        Bitboard rq = pieces[attacker][ROOK] | pieces[attacker][QUEEN];
        Bitboard bq = pieces[attacker][BISHOP] | pieces[attacker][QUEEN];

        int dirs[8] = {8, -8, 1, -1, 9, 7, -7, -9};
        
        for (int i = 0; i < 8; ++i) {
            int dir = dirs[i];
            bool is_diagonal = (i >= 4);
            Bitboard attackers = is_diagonal ? bq : rq;
            
            if (!attackers) continue; 

            int temp_sq = square;
            while (true) {
                if ((dir == 1 || dir == 9 || dir == -7) && (temp_sq % 8 == 7)) break; 
                if ((dir == -1 || dir == 7 || dir == -9) && (temp_sq % 8 == 0)) break; 
                if ((dir == 8 || dir == 9 || dir == 7) && (temp_sq / 8 == 7)) break; 
                if ((dir == -8 || dir == -7 || dir == -9) && (temp_sq / 8 == 0)) break;

                temp_sq += dir;
                
                Bitboard target_bb = 1ULL << temp_sq;
                if (target_bb & attackers) return true; 
                if (target_bb & occ) break;             
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
    }
};
