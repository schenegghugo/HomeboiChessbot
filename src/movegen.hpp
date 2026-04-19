#pragma once
#include "types.hpp"
#include "board.hpp"
#include "magics.hpp"

// A fast array-based list to store generated moves
struct MoveList {
    Move moves[256];
    int count = 0;

    void add(int piece, int from, int to, int captured = EMPTY, int promoted = EMPTY, bool is_castling = false) {
        moves[count++] = {piece, from, to, captured, promoted, is_castling};
    }
};

// Helper to determine which piece is sitting on a square (for captures)
inline int get_piece_at(const Board& board, int sq) {
    for (int p = wP; p <= bK; ++p) {
        if (get_bit(board.pieces[p], sq)) return p;
    }
    return EMPTY;
}

inline MoveList generate_pseudo_legal_moves(const Board& board) {
    MoveList list;
    int stm = board.side_to_move;
    
    U64 us = board.colors[stm];
    U64 them = board.colors[stm ^ 1];
    U64 all_pieces = us | them;
    U64 empty = ~all_pieces;

    int p_offset = (stm == WHITE) ? 0 : 6; // To select wP vs bP, etc.

    // ==========================================
    // 1. PAWNS
    // ==========================================
    U64 pawns = board.pieces[wP + p_offset];
    int push_dir = (stm == WHITE) ? 8 : -8;
    U64 promotion_rank = (stm == WHITE) ? 0xFF00000000000000ULL : 0x00000000000000FFULL;

    while (pawns) {
        int sq = pop_lsb(pawns);
        int target = sq + push_dir;

        // Single Push
        if (get_bit(empty, target)) {
            if ((1ULL << target) & promotion_rank) {
                list.add(wP + p_offset, sq, target, EMPTY, wQ + p_offset);
                list.add(wP + p_offset, sq, target, EMPTY, wR + p_offset);
                list.add(wP + p_offset, sq, target, EMPTY, wB + p_offset);
                list.add(wP + p_offset, sq, target, EMPTY, wN + p_offset);
            } else {
                list.add(wP + p_offset, sq, target);
                
                // Double Push (Only if single push is empty)
                int start_rank = (stm == WHITE) ? 1 : 6;
                if (sq / 8 == start_rank) {
                    int double_target = sq + (push_dir * 2);
                    if (get_bit(empty, double_target)) {
                        list.add(wP + p_offset, sq, double_target);
                    }
                }
            }
        }
        
        // Pawn Captures (Diagonal)
        U64 attacks = (stm == WHITE) ? 
            (((1ULL << sq) << 7) & 0x7f7f7f7f7f7f7f7fULL) | (((1ULL << sq) << 9) & 0xfefefefefefefefeULL) :
            (((1ULL << sq) >> 9) & 0x7f7f7f7f7f7f7f7fULL) | (((1ULL << sq) >> 7) & 0xfefefefefefefefeULL);
        
        U64 valid_captures = attacks & them; // Can only capture enemy pieces

        while (valid_captures) {
            int cap_sq = pop_lsb(valid_captures);
            int captured_piece = get_piece_at(board, cap_sq);
            if ((1ULL << cap_sq) & promotion_rank) {
                list.add(wP + p_offset, sq, cap_sq, captured_piece, wQ + p_offset);
                list.add(wP + p_offset, sq, cap_sq, captured_piece, wR + p_offset);
                list.add(wP + p_offset, sq, cap_sq, captured_piece, wB + p_offset);
                list.add(wP + p_offset, sq, cap_sq, captured_piece, wN + p_offset);
            } else {
                list.add(wP + p_offset, sq, cap_sq, captured_piece);
            }
        }
    }

    // 1b. EN PASSANT
    if (board.ep_square != EMPTY) {
        U64 ep_target = 1ULL << board.ep_square;
        // Find if any of our pawns can attack the EP square (Reverse lookup)
        U64 ep_attackers = (stm == WHITE) ? 
            (((ep_target >> 7) & 0xfefefefefefefefeULL) | ((ep_target >> 9) & 0x7f7f7f7f7f7f7f7fULL)) :
            (((ep_target << 9) & 0xfefefefefefefefeULL) | ((ep_target << 7) & 0x7f7f7f7f7f7f7f7fULL));
        
        U64 valid_attackers = ep_attackers & board.pieces[wP + p_offset];
        while (valid_attackers) {
            int sq = pop_lsb(valid_attackers);
            int captured_pawn = wP + (stm == WHITE ? 6 : 0); // Enemy pawn
            list.add(wP + p_offset, sq, board.ep_square, captured_pawn);
        }
    }

    // ==========================================
    // 2. KNIGHTS & KINGS
    // ==========================================
    U64 knights = board.pieces[wN + p_offset];
    while (knights) {
        int sq = pop_lsb(knights);
        U64 attacks = KnightAttacks[sq] & ~us; // Attack anywhere except our own pieces
        while (attacks) {
            int to = pop_lsb(attacks);
            list.add(wN + p_offset, sq, to, get_piece_at(board, to));
        }
    }

    U64 king = board.pieces[wK + p_offset];
    if (king) {
        int sq = __builtin_ctzll(king);
        U64 attacks = KingAttacks[sq] & ~us;
        while (attacks) {
            int to = pop_lsb(attacks);
            list.add(wK + p_offset, sq, to, get_piece_at(board, to));
        }
    }

    // ==========================================
    // 2b. CASTLING
    // ==========================================
    // (Checks if squares between King and Rook are empty)
    // Assuming castling_rights are formatted: 1 = WK, 2 = WQ, 4 = BK, 8 = BQ
    if (stm == WHITE) {
        // White Kingside Castling (e1 to g1)
        if (board.castling_rights & 1) { 
            if (!get_bit(all_pieces, 5) && !get_bit(all_pieces, 6)) { // f1 (5) and g1 (6) must be empty
                list.add(wK + p_offset, 4, 6, EMPTY, EMPTY, true);
            }
        }
        // White Queenside Castling (e1 to c1)
        if (board.castling_rights & 2) { 
            if (!get_bit(all_pieces, 1) && !get_bit(all_pieces, 2) && !get_bit(all_pieces, 3)) { // b1(1), c1(2), d1(3)
                list.add(wK + p_offset, 4, 2, EMPTY, EMPTY, true);
            }
        }
    } else {
        // Black Kingside Castling (e8 to g8)
        if (board.castling_rights & 4) { 
            if (!get_bit(all_pieces, 61) && !get_bit(all_pieces, 62)) { // f8(61) and g8(62)
                list.add(wK + p_offset, 60, 62, EMPTY, EMPTY, true);
            }
        }
        // Black Queenside Castling (e8 to c8)
        if (board.castling_rights & 8) { 
            if (!get_bit(all_pieces, 57) && !get_bit(all_pieces, 58) && !get_bit(all_pieces, 59)) { // b8(57), c8(58), d8(59)
                list.add(wK + p_offset, 60, 58, EMPTY, EMPTY, true);
            }
        }
    }

    // ==========================================
    // 3. SLIDING PIECES
    // ==========================================
    U64 bishops = board.pieces[wB + p_offset];
    while (bishops) {
        int sq = pop_lsb(bishops);
        U64 attacks = get_bishop_attacks(sq, all_pieces) & ~us;
        while (attacks) {
            int to = pop_lsb(attacks);
            list.add(wB + p_offset, sq, to, get_piece_at(board, to));
        }
    }

    U64 rooks = board.pieces[wR + p_offset];
    while (rooks) {
        int sq = pop_lsb(rooks);
        U64 attacks = get_rook_attacks(sq, all_pieces) & ~us;
        while (attacks) {
            int to = pop_lsb(attacks);
            list.add(wR + p_offset, sq, to, get_piece_at(board, to));
        }
    }

    U64 queens = board.pieces[wQ + p_offset];
    while (queens) {
        int sq = pop_lsb(queens);
        U64 attacks = get_queen_attacks(sq, all_pieces) & ~us;
        while (attacks) {
            int to = pop_lsb(attacks);
            list.add(wQ + p_offset, sq, to, get_piece_at(board, to));
        }
    }

    return list;
}
