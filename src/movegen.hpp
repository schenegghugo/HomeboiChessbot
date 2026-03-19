#pragma once
#include "board.hpp"
#include "magics.hpp"

namespace MoveGen {

const Bitboard FILE_A = 0x0101010101010101ULL;
const Bitboard FILE_H = 0x8080808080808080ULL;

inline bool is_square_attacked(Square sq, Color attacker_color, const Board& board) {
    int attacker = static_cast<int>(attacker_color);
    int sq_idx = static_cast<int>(sq);
    
    Bitboard both_occ = board.occupancies[2] & ~(1ULL << sq_idx);
    Bitboard sq_bb = 1ULL << sq_idx;
    
    if (attacker_color == Color::White) {
        Bitboard pawn_attacks = ((sq_bb >> 7) & ~FILE_A) | ((sq_bb >> 9) & ~FILE_H);
        if (pawn_attacks & board.pieces[attacker][PAWN]) return true;
    } else {
        Bitboard pawn_attacks = ((sq_bb << 7) & ~FILE_H) | ((sq_bb << 9) & ~FILE_A);
        if (pawn_attacks & board.pieces[attacker][PAWN]) return true;
    }

    if (Attacks::KNIGHT_ATTACKS[sq_idx] & board.pieces[attacker][KNIGHT]) return true;
    if (Attacks::KING_ATTACKS[sq_idx] & board.pieces[attacker][KING]) return true;

    Bitboard bishop_attacks = Magics::get_bishop_attacks(static_cast<Square>(sq_idx), both_occ);
    if (bishop_attacks & (board.pieces[attacker][BISHOP] | board.pieces[attacker][QUEEN])) return true;

    Bitboard rook_attacks = Magics::get_rook_attacks(static_cast<Square>(sq_idx), both_occ);
    if (rook_attacks & (board.pieces[attacker][ROOK] | board.pieces[attacker][QUEEN])) return true;

    return false;
}

// --- NEW 16-BIT MOVE ENCODING ---
typedef uint16_t Move;

inline Move encode_move(int src, int tgt, int flag) {
    return (src & 0x3F) | ((tgt & 0x3F) << 6) | ((flag & 0xF) << 12);
}

inline int get_source(Move move) { return move & 0x3F; }
inline int get_target(Move move) { return (move >> 6) & 0x3F; }
inline int get_flag(Move move) { return (move >> 12) & 0xF; }

// Bitwise Magic: Flags 4, 5, 12, 13, 14, 15 all have the '4' bit set.
inline bool is_capture(Move move) { return get_flag(move) & 4; }

// Flags 8 through 15 all have the '8' bit set.
inline bool is_promotion(Move move) { return get_flag(move) & 8; }

// Extracts the piece type. Assuming PAWN=0, KNIGHT=1, BISHOP=2, ROOK=3, QUEEN=4
// Example: Queen Promo is flag 11 (1011). 11 & 3 = 3. 3 + 1 = 4 (QUEEN).
inline int get_promoted_piece(Move move) { return (get_flag(move) & 3) + 1; }

inline std::string move_to_string(Move move) {
    int src = get_source(move);
    int tgt = get_target(move);
    
    char f1 = 'a' + (src % 8);
    char r1 = '1' + (src / 8);
    char f2 = 'a' + (tgt % 8);
    char r2 = '1' + (tgt / 8);

    std::string s = "";
    s += f1; s += r1; s += f2; s += r2;

    if (is_promotion(move)) {
        int p = get_promoted_piece(move);
        if (p == QUEEN) s += "q";
        else if (p == ROOK) s += "r";
        else if (p == BISHOP) s += "b";
        else if (p == KNIGHT) s += "n";
    }
    return s;
}

// Memory footprint drops from 4096 bytes down to 516 bytes!
struct MoveList {
    Move moves[256];
    int count = 0;

    void add_move(int src, int tgt, int flag = 0) {
        moves[count++] = encode_move(src, tgt, flag);
    }
};

inline MoveList generate_pseudo_legal_moves(const Board& board) {
    MoveList move_list;
    Color us = board.side_to_move;
    Color them = (us == Color::White) ? Color::Black : Color::White;

    int us_idx = static_cast<int>(us);
    int them_idx = static_cast<int>(them);

    Bitboard empty = ~board.occupancies[static_cast<int>(Color::Both)];
    Bitboard valid_squares = ~board.occupancies[us_idx];
    Bitboard enemies = board.occupancies[them_idx];
    Bitboard both_occ = board.occupancies[static_cast<int>(Color::Both)];

    auto add_pawn_moves = [&](int src, int tgt, bool is_cap) {
        if (tgt >= 56 || tgt <= 7) {
            int base_flag = is_cap ? 12 : 8; // 12 = PromoCap, 8 = Promo Quiet
            move_list.add_move(src, tgt, base_flag + 3); // Queen
            move_list.add_move(src, tgt, base_flag + 2); // Rook
            move_list.add_move(src, tgt, base_flag + 1); // Bishop
            move_list.add_move(src, tgt, base_flag + 0); // Knight
        } else {
            move_list.add_move(src, tgt, is_cap ? 4 : 0); // 4 = Capture, 0 = Quiet
        }
    };

    if (us == Color::White) {
        Bitboard pawns = board.pieces[us_idx][PAWN];

        Bitboard single_pushes = (pawns << 8) & empty;
        Bitboard double_pushes = ((single_pushes & 0x0000000000FF0000ULL) << 8) & empty;

        Bitboard sp = single_pushes;
        while (sp) {
            int tgt = __builtin_ctzll(sp);
            add_pawn_moves(tgt - 8, tgt, false);
            sp &= sp - 1;
        }

        Bitboard dp = double_pushes;
        while (dp) {
            int tgt = __builtin_ctzll(dp);
            move_list.add_move(tgt - 16, tgt, 1); // 1 = Double Push
            dp &= dp - 1;
        }

        Bitboard left_caps = ((pawns << 7) & ~FILE_H) & enemies;
        Bitboard right_caps = ((pawns << 9) & ~FILE_A) & enemies;

        while (left_caps) {
            int tgt = __builtin_ctzll(left_caps);
            add_pawn_moves(tgt - 7, tgt, true);
            left_caps &= left_caps - 1;
        }
        while (right_caps) {
            int tgt = __builtin_ctzll(right_caps);
            add_pawn_moves(tgt - 9, tgt, true);
            right_caps &= right_caps - 1;
        }

        if (board.en_passant_square != -1) {
            Bitboard ep_sq_bb = 1ULL << board.en_passant_square;
            Bitboard attackers = (((ep_sq_bb >> 7) & ~FILE_A) | ((ep_sq_bb >> 9) & ~FILE_H)) & pawns;
            while (attackers) {
                int src = __builtin_ctzll(attackers);
                move_list.add_move(src, board.en_passant_square, 5); // 5 = En Passant
                attackers &= attackers - 1;
            }
        }

        if (board.castling_rights & CASTLE_WK) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::f1)) | (1ULL << static_cast<int>(Square::g1))))) {
                if (!is_square_attacked(Square::e1, Color::Black, board) && !is_square_attacked(Square::f1, Color::Black, board) && !is_square_attacked(Square::g1, Color::Black, board)) {
                    move_list.add_move(static_cast<int>(Square::e1), static_cast<int>(Square::g1), 2); // 2 = Castle
                }
            }
        }
        if (board.castling_rights & CASTLE_WQ) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::b1)) | (1ULL << static_cast<int>(Square::c1)) | (1ULL << static_cast<int>(Square::d1))))) {
                if (!is_square_attacked(Square::e1, Color::Black, board) && !is_square_attacked(Square::d1, Color::Black, board) && !is_square_attacked(Square::c1, Color::Black, board)) {
                    move_list.add_move(static_cast<int>(Square::e1), static_cast<int>(Square::c1), 2);
                }
            }
        }

    } else {
        Bitboard pawns = board.pieces[us_idx][PAWN];

        Bitboard single_pushes = (pawns >> 8) & empty;
        Bitboard double_pushes = ((single_pushes & 0x0000FF0000000000ULL) >> 8) & empty;

        Bitboard sp = single_pushes;
        while (sp) {
            int tgt = __builtin_ctzll(sp);
            add_pawn_moves(tgt + 8, tgt, false);
            sp &= sp - 1;
        }

        Bitboard dp = double_pushes;
        while (dp) {
            int tgt = __builtin_ctzll(dp);
            move_list.add_move(tgt + 16, tgt, 1);
            dp &= dp - 1;
        }

        Bitboard left_caps = ((pawns >> 9) & ~FILE_H) & enemies;
        Bitboard right_caps = ((pawns >> 7) & ~FILE_A) & enemies;

        while (left_caps) {
            int tgt = __builtin_ctzll(left_caps);
            add_pawn_moves(tgt + 9, tgt, true);
            left_caps &= left_caps - 1;
        }
        while (right_caps) {
            int tgt = __builtin_ctzll(right_caps);
            add_pawn_moves(tgt + 7, tgt, true);
            right_caps &= right_caps - 1;
        }

        if (board.en_passant_square != -1) {
            Bitboard ep_sq_bb = 1ULL << board.en_passant_square;
            Bitboard attackers = (((ep_sq_bb << 9) & ~FILE_A) | ((ep_sq_bb << 7) & ~FILE_H)) & pawns;
            while (attackers) {
                int src = __builtin_ctzll(attackers);
                move_list.add_move(src, board.en_passant_square, 5);
                attackers &= attackers - 1;
            }
        }

        if (board.castling_rights & CASTLE_BK) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::f8)) | (1ULL << static_cast<int>(Square::g8))))) {
                if (!is_square_attacked(Square::e8, Color::White, board) && !is_square_attacked(Square::f8, Color::White, board) && !is_square_attacked(Square::g8, Color::White, board)) {
                    move_list.add_move(static_cast<int>(Square::e8), static_cast<int>(Square::g8), 2);
                }
            }
        }
        if (board.castling_rights & CASTLE_BQ) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::b8)) | (1ULL << static_cast<int>(Square::c8)) | (1ULL << static_cast<int>(Square::d8))))) {
                if (!is_square_attacked(Square::e8, Color::White, board) && !is_square_attacked(Square::d8, Color::White, board) && !is_square_attacked(Square::c8, Color::White, board)) {
                    move_list.add_move(static_cast<int>(Square::e8), static_cast<int>(Square::c8), 2);
                }
            }
        }
    }

    Bitboard knights = board.pieces[us_idx][KNIGHT];
    while (knights) {
        int src = __builtin_ctzll(knights);
        Bitboard attacks = Attacks::KNIGHT_ATTACKS[src] & valid_squares;
        while (attacks) {
            int tgt = __builtin_ctzll(attacks);
            move_list.add_move(src, tgt, ((1ULL << tgt) & enemies) ? 4 : 0);
            attacks &= attacks - 1;
        }
        knights &= knights - 1;
    }

    Bitboard kings = board.pieces[us_idx][KING];
    while (kings) {
        int src = __builtin_ctzll(kings);
        Bitboard attacks = Attacks::KING_ATTACKS[src] & valid_squares;
        while (attacks) {
            int tgt = __builtin_ctzll(attacks);
            move_list.add_move(src, tgt, ((1ULL << tgt) & enemies) ? 4 : 0);
            attacks &= attacks - 1;
        }
        kings &= kings - 1;
    }

    Bitboard bishops = board.pieces[us_idx][BISHOP] | board.pieces[us_idx][QUEEN];
    while (bishops) {
        int src = __builtin_ctzll(bishops);
        Bitboard attacks = Magics::get_bishop_attacks(static_cast<Square>(src), both_occ) & valid_squares;
        while (attacks) {
            int tgt = __builtin_ctzll(attacks);
            move_list.add_move(src, tgt, ((1ULL << tgt) & enemies) ? 4 : 0);
            attacks &= attacks - 1;
        }
        bishops &= bishops - 1;
    }

    Bitboard rooks = board.pieces[us_idx][ROOK] | board.pieces[us_idx][QUEEN];
    while (rooks) {
        int src = __builtin_ctzll(rooks);
        Bitboard attacks = Magics::get_rook_attacks(static_cast<Square>(src), both_occ) & valid_squares;
        while (attacks) {
            int tgt = __builtin_ctzll(attacks);
            move_list.add_move(src, tgt, ((1ULL << tgt) & enemies) ? 4 : 0);
            attacks &= attacks - 1;
        }
        rooks &= rooks - 1;
    }

    return move_list;
}

inline MoveList generate_captures(const Board& board) {
    MoveList move_list;
    Color us = board.side_to_move;
    Color them = (us == Color::White) ? Color::Black : Color::White;

    int us_idx = static_cast<int>(us);
    int them_idx = static_cast<int>(them);

    Bitboard enemies = board.occupancies[them_idx];
    Bitboard both_occ = board.occupancies[static_cast<int>(Color::Both)];

    auto add_capture_or_promo = [&](int src, int tgt, bool is_cap) {
        if (tgt >= 56 || tgt <= 7) {
            int base_flag = is_cap ? 12 : 8;
            move_list.add_move(src, tgt, base_flag + 3);
            move_list.add_move(src, tgt, base_flag + 2);
            move_list.add_move(src, tgt, base_flag + 1);
            move_list.add_move(src, tgt, base_flag + 0);
        } else if (is_cap) {
            move_list.add_move(src, tgt, 4);
        }
    };

    if (us == Color::White) {
        Bitboard pawns = board.pieces[us_idx][PAWN];
        
        Bitboard single_pushes = (pawns << 8) & ~both_occ;
        Bitboard promotions = single_pushes & 0xFF00000000000000ULL;
        while (promotions) {
            int tgt = __builtin_ctzll(promotions);
            add_capture_or_promo(tgt - 8, tgt, false);
            promotions &= promotions - 1;
        }

        Bitboard left_caps = ((pawns << 7) & ~FILE_H) & enemies;
        Bitboard right_caps = ((pawns << 9) & ~FILE_A) & enemies;

        while (left_caps) {
            int tgt = __builtin_ctzll(left_caps);
            add_capture_or_promo(tgt - 7, tgt, true);
            left_caps &= left_caps - 1;
        }
        while (right_caps) {
            int tgt = __builtin_ctzll(right_caps);
            add_capture_or_promo(tgt - 9, tgt, true);
            right_caps &= right_caps - 1;
        }

        if (board.en_passant_square != -1) {
            Bitboard ep_sq_bb = 1ULL << board.en_passant_square;
            Bitboard attackers = (((ep_sq_bb >> 7) & ~FILE_A) | ((ep_sq_bb >> 9) & ~FILE_H)) & pawns;
            while (attackers) {
                move_list.add_move(__builtin_ctzll(attackers), board.en_passant_square, 5);
                attackers &= attackers - 1;
            }
        }
    } else {
        Bitboard pawns = board.pieces[us_idx][PAWN];
        
        Bitboard single_pushes = (pawns >> 8) & ~both_occ;
        Bitboard promotions = single_pushes & 0x00000000000000FFULL;
        while (promotions) {
            int tgt = __builtin_ctzll(promotions);
            add_capture_or_promo(tgt + 8, tgt, false);
            promotions &= promotions - 1;
        }

        Bitboard left_caps = ((pawns >> 9) & ~FILE_H) & enemies;
        Bitboard right_caps = ((pawns >> 7) & ~FILE_A) & enemies;

        while (left_caps) {
            int tgt = __builtin_ctzll(left_caps);
            add_capture_or_promo(tgt + 9, tgt, true);
            left_caps &= left_caps - 1;
        }
        while (right_caps) {
            int tgt = __builtin_ctzll(right_caps);
            add_capture_or_promo(tgt + 7, tgt, true);
            right_caps &= right_caps - 1;
        }

        if (board.en_passant_square != -1) {
            Bitboard ep_sq_bb = 1ULL << board.en_passant_square;
            Bitboard attackers = (((ep_sq_bb << 9) & ~FILE_A) | ((ep_sq_bb << 7) & ~FILE_H)) & pawns;
            while (attackers) {
                move_list.add_move(__builtin_ctzll(attackers), board.en_passant_square, 5);
                attackers &= attackers - 1;
            }
        }
    }

    Bitboard knights = board.pieces[us_idx][KNIGHT];
    while (knights) {
        int src = __builtin_ctzll(knights);
        Bitboard attacks = Attacks::KNIGHT_ATTACKS[src] & enemies;
        while (attacks) {
            move_list.add_move(src, __builtin_ctzll(attacks), 4);
            attacks &= attacks - 1;
        }
        knights &= knights - 1;
    }

    Bitboard kings = board.pieces[us_idx][KING];
    while (kings) {
        int src = __builtin_ctzll(kings);
        Bitboard attacks = Attacks::KING_ATTACKS[src] & enemies;
        while (attacks) {
            move_list.add_move(src, __builtin_ctzll(attacks), 4);
            attacks &= attacks - 1;
        }
        kings &= kings - 1;
    }

    Bitboard bishops = board.pieces[us_idx][BISHOP] | board.pieces[us_idx][QUEEN];
    while (bishops) {
        int src = __builtin_ctzll(bishops);
        Bitboard attacks = Magics::get_bishop_attacks(static_cast<Square>(src), both_occ) & enemies;
        while (attacks) {
            move_list.add_move(src, __builtin_ctzll(attacks), 4);
            attacks &= attacks - 1;
        }
        bishops &= bishops - 1;
    }

    Bitboard rooks = board.pieces[us_idx][ROOK] | board.pieces[us_idx][QUEEN];
    while (rooks) {
        int src = __builtin_ctzll(rooks);
        Bitboard attacks = Magics::get_rook_attacks(static_cast<Square>(src), both_occ) & enemies;
        while (attacks) {
            move_list.add_move(src, __builtin_ctzll(attacks), 4);
            attacks &= attacks - 1;
        }
        rooks &= rooks - 1;
    }

    return move_list;
}

// --- FULLY O(1) UNMAKE MOVE ---
inline void unmake_move(Board& board, Move move) {
    board.history_ply--;
    int ply = board.history_ply;
    
    int src = get_source(move);
    int tgt = get_target(move);
    int flag = get_flag(move);
    bool is_ep = (flag == 5);
    bool is_castle = (flag == 2);
    int promo_piece = is_promotion(move) ? get_promoted_piece(move) : -1;

    int us = static_cast<int>(board.side_to_move) ^ 1;
    int them = us ^ 1;
    
    Bitboard src_bb = 1ULL << src;
    Bitboard tgt_bb = 1ULL << tgt;
    
    board.side_to_move = static_cast<Color>(us);
    
    // --- O(1) LOOKUP! ---
    // If it was a promotion, the piece that moved was a Pawn.
    // Otherwise, the piece sitting on 'tgt' right now is the piece that moved.
    int moving_piece = (promo_piece != -1) ? PAWN : board.piece_on[tgt];
    int captured_piece = board.captured_piece_history[ply];
    
    if (promo_piece != -1) {
        board.pieces[us][promo_piece] ^= tgt_bb; 
        board.occupancies[us] ^= tgt_bb;
    } else {
        board.pieces[us][moving_piece] ^= tgt_bb; 
        board.occupancies[us] ^= tgt_bb;
    }
    board.pieces[us][moving_piece] ^= src_bb; 
    board.occupancies[us] ^= src_bb;
    
    // Unmake the piece_on array
    board.piece_on[src] = moving_piece;
    board.piece_on[tgt] = (captured_piece != -1 && !is_ep) ? captured_piece : EMPTY;
    
    if (captured_piece != -1) {
        if (is_ep) {
            int cap_sq = tgt + (us == static_cast<int>(Color::White) ? -8 : 8);
            board.pieces[them][PAWN] ^= (1ULL << cap_sq);
            board.occupancies[them] ^= (1ULL << cap_sq);
            board.piece_on[cap_sq] = PAWN;
        } else {
            board.pieces[them][captured_piece] ^= tgt_bb;
            board.occupancies[them] ^= tgt_bb;
        }
    }
    
    if (is_castle) {
        if (tgt == static_cast<int>(Square::g1)) {
            Bitboard rooks = (1ULL << static_cast<int>(Square::h1)) | (1ULL << static_cast<int>(Square::f1));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.piece_on[static_cast<int>(Square::h1)] = ROOK; board.piece_on[static_cast<int>(Square::f1)] = EMPTY;
        } else if (tgt == static_cast<int>(Square::c1)) {
            Bitboard rooks = (1ULL << static_cast<int>(Square::a1)) | (1ULL << static_cast<int>(Square::d1));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.piece_on[static_cast<int>(Square::a1)] = ROOK; board.piece_on[static_cast<int>(Square::d1)] = EMPTY;
        } else if (tgt == static_cast<int>(Square::g8)) {
            Bitboard rooks = (1ULL << static_cast<int>(Square::h8)) | (1ULL << static_cast<int>(Square::f8));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.piece_on[static_cast<int>(Square::h8)] = ROOK; board.piece_on[static_cast<int>(Square::f8)] = EMPTY;
        } else if (tgt == static_cast<int>(Square::c8)) {
            Bitboard rooks = (1ULL << static_cast<int>(Square::a8)) | (1ULL << static_cast<int>(Square::d8));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.piece_on[static_cast<int>(Square::a8)] = ROOK; board.piece_on[static_cast<int>(Square::d8)] = EMPTY;
        }
    }
    
    board.hash = board.repetition_history[ply];
    board.en_passant_square = board.ep_history[ply];
    board.castling_rights = board.castle_history[ply];
    board.half_move_clock = board.half_move_history[ply];
    board.occupancies[2] = board.occupancies[0] | board.occupancies[1];
}

// --- FULLY O(1) MAKE MOVE ---
inline bool make_move(Board& board, Move move) {
    int src = get_source(move);
    int tgt = get_target(move);
    int flag = get_flag(move);
    bool is_cap = is_capture(move);
    bool is_ep = (flag == 5);
    bool is_castle = (flag == 2);
    bool is_dp = (flag == 1);
    int promo_piece = is_promotion(move) ? get_promoted_piece(move) : -1;

    int us = static_cast<int>(board.side_to_move);
    int them = us ^ 1;
    int ply = board.history_ply;
    Bitboard src_bb = 1ULL << src;
    Bitboard tgt_bb = 1ULL << tgt;
    
    // --- O(1) LOOKUP! No more if/else! ---
    int moving_piece = board.piece_on[src]; 
    
    board.repetition_history[ply] = board.hash;
    board.ep_history[ply] = board.en_passant_square;
    board.castle_history[ply] = board.castling_rights;
    board.half_move_history[ply] = board.half_move_clock;
    
    if (board.en_passant_square != -1) board.hash ^= Zobrist::ep_keys[board.en_passant_square % 8];
    board.hash ^= Zobrist::castle_keys[board.castling_rights];
    
    int captured_piece = -1;
    if (is_cap) {
        if (is_ep) {
            captured_piece = PAWN;
            int cap_sq = tgt + (us == static_cast<int>(Color::White) ? -8 : 8);
            board.pieces[them][PAWN] ^= (1ULL << cap_sq);
            board.occupancies[them] ^= (1ULL << cap_sq);
            board.hash ^= Zobrist::piece_keys[them][PAWN][cap_sq];
            board.piece_on[cap_sq] = EMPTY; // Clear captured EP pawn from array
        } else {
            // --- O(1) LOOKUP! ---
            captured_piece = board.piece_on[tgt];
            
            board.pieces[them][captured_piece] ^= tgt_bb;
            board.occupancies[them] ^= tgt_bb;
            board.hash ^= Zobrist::piece_keys[them][captured_piece][tgt];
        }
        board.half_move_clock = 0;
    } else if (moving_piece == PAWN) {
        board.half_move_clock = 0;
    } else {
        board.half_move_clock++;
    }
    board.captured_piece_history[ply] = captured_piece;
    
    board.pieces[us][moving_piece] ^= src_bb;
    board.occupancies[us] ^= src_bb;
    board.hash ^= Zobrist::piece_keys[us][moving_piece][src];
    
    if (promo_piece != -1) {
        board.pieces[us][promo_piece] ^= tgt_bb;
        board.occupancies[us] ^= tgt_bb;
        board.hash ^= Zobrist::piece_keys[us][promo_piece][tgt];
    } else {
        board.pieces[us][moving_piece] ^= tgt_bb;
        board.occupancies[us] ^= tgt_bb;
        board.hash ^= Zobrist::piece_keys[us][moving_piece][tgt];
    }
    
    // Update the piece_on array
    board.piece_on[src] = EMPTY;
    board.piece_on[tgt] = (promo_piece != -1) ? promo_piece : moving_piece;
    
    if (is_castle) {
        if (tgt == static_cast<int>(Square::g1)) { 
            Bitboard rooks = (1ULL << static_cast<int>(Square::h1)) | (1ULL << static_cast<int>(Square::f1));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.hash ^= Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::h1)] ^ Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::f1)];
            board.piece_on[static_cast<int>(Square::h1)] = EMPTY; board.piece_on[static_cast<int>(Square::f1)] = ROOK;
        } else if (tgt == static_cast<int>(Square::c1)) { 
            Bitboard rooks = (1ULL << static_cast<int>(Square::a1)) | (1ULL << static_cast<int>(Square::d1));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.hash ^= Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::a1)] ^ Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::d1)];
            board.piece_on[static_cast<int>(Square::a1)] = EMPTY; board.piece_on[static_cast<int>(Square::d1)] = ROOK;
        } else if (tgt == static_cast<int>(Square::g8)) { 
            Bitboard rooks = (1ULL << static_cast<int>(Square::h8)) | (1ULL << static_cast<int>(Square::f8));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.hash ^= Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::h8)] ^ Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::f8)];
            board.piece_on[static_cast<int>(Square::h8)] = EMPTY; board.piece_on[static_cast<int>(Square::f8)] = ROOK;
        } else if (tgt == static_cast<int>(Square::c8)) { 
            Bitboard rooks = (1ULL << static_cast<int>(Square::a8)) | (1ULL << static_cast<int>(Square::d8));
            board.pieces[us][ROOK] ^= rooks; board.occupancies[us] ^= rooks;
            board.hash ^= Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::a8)] ^ Zobrist::piece_keys[us][ROOK][static_cast<int>(Square::d8)];
            board.piece_on[static_cast<int>(Square::a8)] = EMPTY; board.piece_on[static_cast<int>(Square::d8)] = ROOK;
        }
    }
    
    if (is_dp) board.en_passant_square = tgt + (us == static_cast<int>(Color::White) ? -8 : 8);
    else board.en_passant_square = -1;
    
    if (board.en_passant_square != -1) board.hash ^= Zobrist::ep_keys[board.en_passant_square % 8];
    
    board.castling_rights &= Zobrist::castling_mask[src] & Zobrist::castling_mask[tgt];
    board.hash ^= Zobrist::castle_keys[board.castling_rights];
    
    board.occupancies[2] = board.occupancies[0] | board.occupancies[1];
    board.side_to_move = static_cast<Color>(them);
    board.hash ^= Zobrist::side_key;
    board.history_ply++;
    
    int king_sq = __builtin_ctzll(board.pieces[us][KING]);
    if (is_square_attacked(static_cast<Square>(king_sq), static_cast<Color>(them), board)) {
        unmake_move(board, move);
        return false;
    }
    
    return true; 
}

} // namespace MoveGen
