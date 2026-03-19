#pragma once
#include "bitboard.hpp"
#include <iostream>
#include <string>
#include <random>

constexpr int PAWN = 0;
constexpr int KNIGHT = 1;
constexpr int BISHOP = 2;
constexpr int ROOK = 3;
constexpr int QUEEN = 4;
constexpr int KING = 5;
constexpr int EMPTY = 6; // --- NEW: Represents an empty square ---

constexpr int CASTLE_WK = 1;
constexpr int CASTLE_WQ = 2;
constexpr int CASTLE_BK = 4;
constexpr int CASTLE_BQ = 8;

namespace Zobrist {
    inline uint64_t piece_keys[2][6][64];
    inline uint64_t side_key;
    inline uint64_t ep_keys[8]; 
    inline uint64_t castle_keys[16];
    inline int castling_mask[64];

    inline void init() {
        std::mt19937_64 rng(12345);
        for (int c = 0; c < 2; c++)
            for (int p = 0; p < 6; p++)
                for (int s = 0; s < 64; s++)
                    piece_keys[c][p][s] = rng();
        
        side_key = rng();
        for (int i = 0; i < 8; i++) ep_keys[i] = rng();
        for (int i = 0; i < 16; i++) castle_keys[i] = rng();

        for (int i = 0; i < 64; i++) castling_mask[i] = 15;
        castling_mask[7] &= ~CASTLE_WK;   // h1
        castling_mask[0] &= ~CASTLE_WQ;   // a1
        castling_mask[4] &= ~(CASTLE_WK | CASTLE_WQ); // e1
        castling_mask[63] &= ~CASTLE_BK;  // h8
        castling_mask[56] &= ~CASTLE_BQ;  // a8
        castling_mask[60] &= ~(CASTLE_BK | CASTLE_BQ); // e8
    }
}

struct Board {
    Bitboard pieces[2][6];
    Bitboard occupancies[3];
    Color side_to_move;

    int castling_rights;
    int en_passant_square;
    int half_move_clock;
    int history_ply;

    // --- NEW: O(1) Piece Lookup Array ---
    int piece_on[64];

    // --- State History Arrays for O(1) Undo ---
    uint64_t hash;
    uint64_t repetition_history[2048];
    int ep_history[2048];
    int castle_history[2048];
    int half_move_history[2048];
    int captured_piece_history[2048];

    void compute_initial_hash() {
        hash = 0;
        for (int c = 0; c < 2; c++) {
            for (int p = 0; p < 6; p++) {
                Bitboard bb = pieces[c][p];
                while (bb) {
                    int sq = __builtin_ctzll(bb);
                    hash ^= Zobrist::piece_keys[c][p][sq];
                    bb &= (bb - 1);
                }
            }
        }
        if (side_to_move == Color::Black) hash ^= Zobrist::side_key;
        if (en_passant_square != -1) hash ^= Zobrist::ep_keys[en_passant_square % 8];
        hash ^= Zobrist::castle_keys[castling_rights];
    }

    // --- NEW: Call this if you ever load a position from a FEN! ---
    void sync_piece_on() {
        for (int i = 0; i < 64; ++i) piece_on[i] = EMPTY;
        for (int c = 0; c < 2; ++c) {
            for (int p = 0; p < 6; ++p) {
                Bitboard bb = pieces[c][p];
                while (bb) {
                    int sq = __builtin_ctzll(bb);
                    piece_on[sq] = p;
                    bb &= (bb - 1);
                }
            }
        }
    }

    void reset() {
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 6; ++j) pieces[i][j] = 0ULL;
        for (int i = 0; i < 3; ++i) occupancies[i] = 0ULL;
        for (int i = 0; i < 64; ++i) piece_on[i] = EMPTY; // Clear array
        side_to_move = Color::White;
        castling_rights = 0;
        en_passant_square = -1;
        half_move_clock = 0;
        history_ply = 0;
        hash = 0;
    }

    void set_starting_position() {
        Zobrist::init();
        reset();
        pieces[static_cast<int>(Color::White)][PAWN] = 0x000000000000FF00ULL;
        pieces[static_cast<int>(Color::White)][KNIGHT] = 0x0000000000000042ULL;
        pieces[static_cast<int>(Color::White)][BISHOP] = 0x0000000000000024ULL;
        pieces[static_cast<int>(Color::White)][ROOK] = 0x0000000000000081ULL;
        pieces[static_cast<int>(Color::White)][QUEEN] = 0x0000000000000008ULL;
        pieces[static_cast<int>(Color::White)][KING] = 0x0000000000000010ULL;

        pieces[static_cast<int>(Color::Black)][PAWN] = 0x00FF000000000000ULL;
        pieces[static_cast<int>(Color::Black)][KNIGHT] = 0x4200000000000000ULL;
        pieces[static_cast<int>(Color::Black)][BISHOP] = 0x2400000000000000ULL;
        pieces[static_cast<int>(Color::Black)][ROOK] = 0x8100000000000000ULL;
        pieces[static_cast<int>(Color::Black)][QUEEN] = 0x0800000000000000ULL;
        pieces[static_cast<int>(Color::Black)][KING] = 0x1000000000000000ULL;

        for (int i = 0; i < 6; ++i) {
            occupancies[static_cast<int>(Color::White)] |= pieces[static_cast<int>(Color::White)][i];
            occupancies[static_cast<int>(Color::Black)] |= pieces[static_cast<int>(Color::Black)][i];
        }
        occupancies[static_cast<int>(Color::Both)] = occupancies[static_cast<int>(Color::White)] | occupancies[static_cast<int>(Color::Black)];
        side_to_move = Color::White;
        castling_rights = CASTLE_WK | CASTLE_WQ | CASTLE_BK | CASTLE_BQ;
        en_passant_square = -1; 
        
        sync_piece_on(); // Fill the piece array!
        compute_initial_hash();
    }
};
