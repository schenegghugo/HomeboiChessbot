#pragma once
#include "types.hpp"
#include "zobrist.hpp" // <-- ADDED ZOBRIST
#include <string>
#include <sstream>
#include <cctype> // For isdigit()
#include <cmath>

// ==========================================
// Fast Bitwise Operations
// ==========================================
inline void set_bit(U64& bb, int sq)   { bb |= (1ULL << sq); }
inline void clear_bit(U64& bb, int sq) { bb &= ~(1ULL << sq); }
inline bool get_bit(U64 bb, int sq)    { return bb & (1ULL << sq); }

// Hardware instruction to find and clear the first 1-bit (returns 0-63)
inline int pop_lsb(U64& bb) {
    int sq = __builtin_ctzll(bb);
    bb &= bb - 1; 
    return sq;
}

// ==========================================
// The Professional Board State
// ==========================================
struct Board {
    U64 pieces[12] = {0}; 
    U64 colors[2] = {0};  
    
    int white_king_sq = e1;
    int black_king_sq = e8;
    int side_to_move = WHITE;

    // Advanced Chess State Variables
    int ep_square = EMPTY;     
    int castling_rights = 0;   
    
    U64 hash = 0; // <-- ADDED HASH

    // 1. Clear the board for a new position
    void reset() {
        for (int i = 0; i < 12; ++i) pieces[i] = 0ULL;
        colors[WHITE] = colors[BLACK] = 0ULL;
        ep_square = EMPTY;
        castling_rights = 0;
        white_king_sq = e1;
        black_king_sq = e8;
        hash = 0;
    }

    // 2. Add a piece to the bitboards
    void add_piece(int piece, int sq) {
        set_bit(pieces[piece], sq);
        set_bit(colors[(piece < 6) ? WHITE : BLACK], sq);
        
        if (piece == wK) white_king_sq = sq;
        if (piece == bK) black_king_sq = sq;
    }
    
    // 3. Initialize Zobrist Hash from scratch (used after loading FEN)
    void calculate_hash() {
        hash = 0;
        for (int p = 0; p < 12; ++p) {
            U64 b = pieces[p];
            while (b) {
                int sq = __builtin_ctzll(b);
                hash ^= Zobrist::piece_keys[p][sq];
                b &= b - 1; // clear LSB
            }
        }
        if (side_to_move == BLACK) hash ^= Zobrist::side_key;
        if (ep_square != EMPTY) hash ^= Zobrist::ep_keys[ep_square];
        hash ^= Zobrist::castle_keys[castling_rights];
    }

    // 4. THE FULLY LEGAL MOVE MAKER
    void make_move(const Move& m) {
        int us = side_to_move;
        int enemy = us ^ 1;

        // A. Remove moving piece from source
        clear_bit(pieces[m.piece], m.from);
        clear_bit(colors[us], m.from);
        hash ^= Zobrist::piece_keys[m.piece][m.from];

        // B. Handle Captures & En Passant captures
        if (m.captured != EMPTY) {
            int cap_sq = m.to;
            if ((m.piece == wP || m.piece == bP) && cap_sq == ep_square) {
                cap_sq = (us == WHITE) ? m.to - 8 : m.to + 8; // Adjust for EP capture square
            }
            clear_bit(pieces[m.captured], cap_sq);
            clear_bit(colors[enemy], cap_sq);
            hash ^= Zobrist::piece_keys[m.captured][cap_sq];
        }

        // C. Handle Castling (Move the Rook)
        if (m.is_castling) {
            int r_from = -1, r_to = -1;
            int r_piece = (us == WHITE) ? wR : bR;
            if (m.to == 6)  { r_from = 7;  r_to = 5; }  // wK O-O
            if (m.to == 2)  { r_from = 0;  r_to = 3; }  // wK O-O-O
            if (m.to == 62) { r_from = 63; r_to = 61; } // bK O-O
            if (m.to == 58) { r_from = 56; r_to = 59; } // bK O-O-O
            
            if (r_from != -1) {
                clear_bit(pieces[r_piece], r_from);
                clear_bit(colors[us], r_from);
                hash ^= Zobrist::piece_keys[r_piece][r_from];
                
                set_bit(pieces[r_piece], r_to);
                set_bit(colors[us], r_to);
                hash ^= Zobrist::piece_keys[r_piece][r_to];
            }
        }

        // D. Handle Promotions
        int placed_piece = (m.promoted != EMPTY) ? m.promoted : m.piece;

        // E. Place piece on target
        set_bit(pieces[placed_piece], m.to);
        set_bit(colors[us], m.to);
        hash ^= Zobrist::piece_keys[placed_piece][m.to];

        if (placed_piece == wK) white_king_sq = m.to;
        if (placed_piece == bK) black_king_sq = m.to;

        // F. Update En Passant state & Hash
        if (ep_square != EMPTY) {
            hash ^= Zobrist::ep_keys[ep_square];
            ep_square = EMPTY;
        }
        if ((m.piece == wP || m.piece == bP) && std::abs(m.to - m.from) == 16) {
            ep_square = (us == WHITE) ? m.from + 8 : m.from - 8;
            hash ^= Zobrist::ep_keys[ep_square];
        }

        // G. Turn ends
        side_to_move ^= 1;
        hash ^= Zobrist::side_key;
        
        // Note: Castling rights update would normally go here for a perfect TT implementation.
    }

    // 5. THE FEN PARSER
    void load_fen(const std::string& fen) {
        reset();
        
        std::istringstream iss(fen);
        std::string board_part, color_part, castling_part, ep_part;
        
        iss >> board_part >> color_part >> castling_part >> ep_part;

        int rank = 7; 
        int file = 0; 
        
        for (char c : board_part) {
            if (c == '/') {
                rank--;   
                file = 0; 
            } else if (std::isdigit(c)) {
                file += (c - '0'); 
            } else {
                int sq = rank * 8 + file;
                add_piece(char_to_piece(c), sq);
                file++;
            }
        }

        side_to_move = (color_part == "w") ? WHITE : BLACK;

        if (castling_part != "-") {
            for (char c : castling_part) {
                if (c == 'K') castling_rights |= 1; 
                if (c == 'Q') castling_rights |= 2; 
                if (c == 'k') castling_rights |= 4; 
                if (c == 'q') castling_rights |= 8; 
            }
        }

        if (ep_part != "-") {
            int ep_file = ep_part[0] - 'a'; 
            int ep_rank = ep_part[1] - '1'; 
            ep_square = ep_rank * 8 + ep_file;
        }
        
        // Finalize Zobrist Hash after setting up the board!
        calculate_hash();
    }
};
