#pragma once
#include "board.hpp"
#include "magics.hpp"

namespace MoveGen {

const Bitboard FILE_A = 0x0101010101010101ULL;
const Bitboard FILE_H = 0x8080808080808080ULL;

inline bool is_square_attacked(Square sq, Color attacker_color, const Board& board) {
    int attacker = static_cast<int>(attacker_color);
    int sq_idx = static_cast<int>(sq);
    Bitboard both_occ = board.occupancies[static_cast<int>(Color::Both)];

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

    Bitboard bishop_attacks = Magics::get_bishop_attacks(sq, both_occ);
    if (bishop_attacks & (board.pieces[attacker][BISHOP] | board.pieces[attacker][QUEEN])) return true;

    Bitboard rook_attacks = Magics::get_rook_attacks(sq, both_occ);
    if (rook_attacks & (board.pieces[attacker][ROOK] | board.pieces[attacker][QUEEN])) return true;

    return false;
}

struct Move {
    Square source;
    Square target;
    bool is_capture = false;
    bool is_double_push = false;
    bool is_en_passant = false;
    bool is_castling = false;
    int promoted_piece = -1;

    std::string to_string() const {
        char f1 = 'a' + (static_cast<int>(source) % 8);
        char r1 = '1' + (static_cast<int>(source) / 8);
        char f2 = 'a' + (static_cast<int>(target) % 8);
        char r2 = '1' + (static_cast<int>(target) / 8);

        std::string s = "";
        s += f1; s += r1; s += f2; s += r2;

        if (promoted_piece == QUEEN) s += "q";
        else if (promoted_piece == ROOK) s += "r";
        else if (promoted_piece == BISHOP) s += "b";
        else if (promoted_piece == KNIGHT) s += "n";

        return s;
    }
};

struct MoveList {
    Move moves[256];
    int count = 0;

    void add_move(Square src, Square tgt, bool capture = false, bool dbl_push = false, bool ep = false, bool castle = false, int promo = -1) {
        moves[count++] = {src, tgt, capture, dbl_push, ep, castle, promo};
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

    auto add_pawn_moves = [&](Square src, Square tgt, bool is_cap) {
        int t = static_cast<int>(tgt);
        if (t >= 56 || t <= 7) {
            move_list.add_move(src, tgt, is_cap, false, false, false, QUEEN);
            move_list.add_move(src, tgt, is_cap, false, false, false, ROOK);
            move_list.add_move(src, tgt, is_cap, false, false, false, BISHOP);
            move_list.add_move(src, tgt, is_cap, false, false, false, KNIGHT);
        } else {
            move_list.add_move(src, tgt, is_cap);
        }
    };

    if (us == Color::White) {
        Bitboard pawns = board.pieces[us_idx][PAWN];

        Bitboard single_pushes = (pawns << 8) & empty;
        Bitboard double_pushes = ((single_pushes & 0x0000000000FF0000ULL) << 8) & empty;

        Bitboard sp = single_pushes;
        while (sp) {
            Square tgt = static_cast<Square>(__builtin_ctzll(sp));
            Square src = static_cast<Square>(static_cast<int>(tgt) - 8);
            add_pawn_moves(src, tgt, false);
            sp &= sp - 1;
        }

        Bitboard dp = double_pushes;
        while (dp) {
            Square tgt = static_cast<Square>(__builtin_ctzll(dp));
            Square src = static_cast<Square>(static_cast<int>(tgt) - 16);
            move_list.add_move(src, tgt, false, true);
            dp &= dp - 1;
        }

        Bitboard left_caps = ((pawns << 7) & ~FILE_H) & enemies;
        Bitboard right_caps = ((pawns << 9) & ~FILE_A) & enemies;

        while (left_caps) {
            Square tgt = static_cast<Square>(__builtin_ctzll(left_caps));
            Square src = static_cast<Square>(static_cast<int>(tgt) - 7);
            add_pawn_moves(src, tgt, true);
            left_caps &= left_caps - 1;
        }
        while (right_caps) {
            Square tgt = static_cast<Square>(__builtin_ctzll(right_caps));
            Square src = static_cast<Square>(static_cast<int>(tgt) - 9);
            add_pawn_moves(src, tgt, true);
            right_caps &= right_caps - 1;
        }

        if (board.en_passant_square != -1) {
            Bitboard ep_sq_bb = 1ULL << board.en_passant_square;
            Bitboard attackers = (((ep_sq_bb >> 7) & ~FILE_A) | ((ep_sq_bb >> 9) & ~FILE_H)) & pawns;
            while (attackers) {
                Square src = static_cast<Square>(__builtin_ctzll(attackers));
                move_list.add_move(src, static_cast<Square>(board.en_passant_square), true, false, true);
                attackers &= attackers - 1;
            }
        }

        if (board.castling_rights & CASTLE_WK) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::f1)) | (1ULL << static_cast<int>(Square::g1))))) {
                if (!is_square_attacked(Square::e1, Color::Black, board) && !is_square_attacked(Square::f1, Color::Black, board) && !is_square_attacked(Square::g1, Color::Black, board)) {
                    move_list.add_move(Square::e1, Square::g1, false, false, false, true);
                }
            }
        }
        if (board.castling_rights & CASTLE_WQ) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::b1)) | (1ULL << static_cast<int>(Square::c1)) | (1ULL << static_cast<int>(Square::d1))))) {
                if (!is_square_attacked(Square::e1, Color::Black, board) && !is_square_attacked(Square::d1, Color::Black, board) && !is_square_attacked(Square::c1, Color::Black, board)) {
                    move_list.add_move(Square::e1, Square::c1, false, false, false, true);
                }
            }
        }

    } else {
        Bitboard pawns = board.pieces[us_idx][PAWN];

        Bitboard single_pushes = (pawns >> 8) & empty;
        Bitboard double_pushes = ((single_pushes & 0x0000FF0000000000ULL) >> 8) & empty;

        Bitboard sp = single_pushes;
        while (sp) {
            Square tgt = static_cast<Square>(__builtin_ctzll(sp));
            Square src = static_cast<Square>(static_cast<int>(tgt) + 8);
            add_pawn_moves(src, tgt, false);
            sp &= sp - 1;
        }

        Bitboard dp = double_pushes;
        while (dp) {
            Square tgt = static_cast<Square>(__builtin_ctzll(dp));
            Square src = static_cast<Square>(static_cast<int>(tgt) + 16);
            move_list.add_move(src, tgt, false, true);
            dp &= dp - 1;
        }

        Bitboard left_caps = ((pawns >> 9) & ~FILE_H) & enemies;
        Bitboard right_caps = ((pawns >> 7) & ~FILE_A) & enemies;

        while (left_caps) {
            Square tgt = static_cast<Square>(__builtin_ctzll(left_caps));
            Square src = static_cast<Square>(static_cast<int>(tgt) + 9);
            add_pawn_moves(src, tgt, true);
            left_caps &= left_caps - 1;
        }
        while (right_caps) {
            Square tgt = static_cast<Square>(__builtin_ctzll(right_caps));
            Square src = static_cast<Square>(static_cast<int>(tgt) + 7);
            add_pawn_moves(src, tgt, true);
            right_caps &= right_caps - 1;
        }

        if (board.en_passant_square != -1) {
            Bitboard ep_sq_bb = 1ULL << board.en_passant_square;
            
            // CRITICAL BUG FIX: The bitmasks for the A-file and H-file were swapped here! 
            // It prevented Black from properly generating valid En Passant captures, 
            // causing the engine to skip the move and fatally desync from the game!
            Bitboard attackers = (((ep_sq_bb << 9) & ~FILE_A) | ((ep_sq_bb << 7) & ~FILE_H)) & pawns;
            
            while (attackers) {
                Square src = static_cast<Square>(__builtin_ctzll(attackers));
                move_list.add_move(src, static_cast<Square>(board.en_passant_square), true, false, true);
                attackers &= attackers - 1;
            }
        }

        if (board.castling_rights & CASTLE_BK) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::f8)) | (1ULL << static_cast<int>(Square::g8))))) {
                if (!is_square_attacked(Square::e8, Color::White, board) && !is_square_attacked(Square::f8, Color::White, board) && !is_square_attacked(Square::g8, Color::White, board)) {
                    move_list.add_move(Square::e8, Square::g8, false, false, false, true);
                }
            }
        }
        if (board.castling_rights & CASTLE_BQ) {
            if (!(both_occ & ((1ULL << static_cast<int>(Square::b8)) | (1ULL << static_cast<int>(Square::c8)) | (1ULL << static_cast<int>(Square::d8))))) {
                if (!is_square_attacked(Square::e8, Color::White, board) && !is_square_attacked(Square::d8, Color::White, board) && !is_square_attacked(Square::c8, Color::White, board)) {
                    move_list.add_move(Square::e8, Square::c8, false, false, false, true);
                }
            }
        }
    }

    // --- LEAPERS AND SLIDERS ---

    Bitboard knights = board.pieces[us_idx][KNIGHT];
    while (knights) {
        Square src = static_cast<Square>(__builtin_ctzll(knights));
        Bitboard attacks = Attacks::KNIGHT_ATTACKS[static_cast<int>(src)] & valid_squares;
        while (attacks) {
            Square tgt = static_cast<Square>(__builtin_ctzll(attacks));
            bool is_cap = (1ULL << static_cast<int>(tgt)) & enemies;
            move_list.add_move(src, tgt, is_cap);
            attacks &= attacks - 1;
        }
        knights &= knights - 1;
    }

    Bitboard kings = board.pieces[us_idx][KING];
    while (kings) {
        Square src = static_cast<Square>(__builtin_ctzll(kings));
        Bitboard attacks = Attacks::KING_ATTACKS[static_cast<int>(src)] & valid_squares;
        while (attacks) {
            Square tgt = static_cast<Square>(__builtin_ctzll(attacks));
            bool is_cap = (1ULL << static_cast<int>(tgt)) & enemies;
            move_list.add_move(src, tgt, is_cap);
            attacks &= attacks - 1;
        }
        kings &= kings - 1;
    }

    Bitboard bishops = board.pieces[us_idx][BISHOP] | board.pieces[us_idx][QUEEN];
    while (bishops) {
        Square src = static_cast<Square>(__builtin_ctzll(bishops));
        Bitboard attacks = Magics::get_bishop_attacks(src, both_occ) & valid_squares;
        while (attacks) {
            Square tgt = static_cast<Square>(__builtin_ctzll(attacks));
            bool is_cap = (1ULL << static_cast<int>(tgt)) & enemies;
            move_list.add_move(src, tgt, is_cap);
            attacks &= attacks - 1;
        }
        bishops &= bishops - 1;
    }

    Bitboard rooks = board.pieces[us_idx][ROOK] | board.pieces[us_idx][QUEEN];
    while (rooks) {
        Square src = static_cast<Square>(__builtin_ctzll(rooks));
        Bitboard attacks = Magics::get_rook_attacks(src, both_occ) & valid_squares;
        while (attacks) {
            Square tgt = static_cast<Square>(__builtin_ctzll(attacks));
            bool is_cap = (1ULL << static_cast<int>(tgt)) & enemies;
            move_list.add_move(src, tgt, is_cap);
            attacks &= attacks - 1;
        }
        rooks &= rooks - 1;
    }

    return move_list;
}

// Returns TRUE if the move is legal. Returns FALSE if it leaves the King in check.
inline bool make_move(Board& board, const Move& move) {
    int us = static_cast<int>(board.side_to_move);
    int them = us ^ 1;

    // 1. Find the moving piece
    int moving_piece = -1;
    for (int i = 0; i < 6; ++i) {
        if (board.pieces[us][i] & (1ULL << static_cast<int>(move.source))) {
            moving_piece = i;
            break;
        }
    }
    if (moving_piece == -1) return false;

    // 2. Move piece on bitboards
    board.pieces[us][moving_piece] &= ~(1ULL << static_cast<int>(move.source));
    board.pieces[us][moving_piece] |= (1ULL << static_cast<int>(move.target));

    // 3. Handle Normal Captures
    if (move.is_capture && !move.is_en_passant) {
        for (int i = 0; i < 6; ++i) {
            if (board.pieces[them][i] & (1ULL << static_cast<int>(move.target))) {
                board.pieces[them][i] &= ~(1ULL << static_cast<int>(move.target));
                break;
            }
        }
    }

    // 4. Handle Promotions
    if (move.promoted_piece != -1) {
        board.pieces[us][PAWN] &= ~(1ULL << static_cast<int>(move.target));
        board.pieces[us][move.promoted_piece] |= (1ULL << static_cast<int>(move.target));
    }

    // 5. Handle En Passant Capture
    if (move.is_en_passant) {
        int capture_square = static_cast<int>(move.target) + (us == static_cast<int>(Color::White) ? -8 : 8);
        board.pieces[them][PAWN] &= ~(1ULL << capture_square);
    }

    // 6. Handle Castling (Moving the Rook)
    if (move.is_castling) {
        int tgt = static_cast<int>(move.target);
        if (tgt == static_cast<int>(Square::g1)) {
            board.pieces[us][ROOK] &= ~(1ULL << static_cast<int>(Square::h1));
            board.pieces[us][ROOK] |= (1ULL << static_cast<int>(Square::f1));
        } else if (tgt == static_cast<int>(Square::c1)) {
            board.pieces[us][ROOK] &= ~(1ULL << static_cast<int>(Square::a1));
            board.pieces[us][ROOK] |= (1ULL << static_cast<int>(Square::d1));
        } else if (tgt == static_cast<int>(Square::g8)) { 
            board.pieces[us][ROOK] &= ~(1ULL << static_cast<int>(Square::h8));
            board.pieces[us][ROOK] |= (1ULL << static_cast<int>(Square::f8));
        } else if (tgt == static_cast<int>(Square::c8)) {
            board.pieces[us][ROOK] &= ~(1ULL << static_cast<int>(Square::a8));
            board.pieces[us][ROOK] |= (1ULL << static_cast<int>(Square::d8));
        }
    }

    // 7. Update En Passant Target Square
    if (move.is_double_push) {
        board.en_passant_square = static_cast<int>(move.target) + (us == static_cast<int>(Color::White) ? -8 : 8);
    } else {
        board.en_passant_square = -1;
    }

    // 8. Update Castling Rights (Loss of rights)
    if (moving_piece == KING) {
        if (us == static_cast<int>(Color::White)) board.castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
        else board.castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
    }
    int src = static_cast<int>(move.source);
    int tgt = static_cast<int>(move.target);
    if (src == static_cast<int>(Square::h1) || tgt == static_cast<int>(Square::h1)) board.castling_rights &= ~CASTLE_WK;
    if (src == static_cast<int>(Square::a1) || tgt == static_cast<int>(Square::a1)) board.castling_rights &= ~CASTLE_WQ;
    if (src == static_cast<int>(Square::h8) || tgt == static_cast<int>(Square::h8)) board.castling_rights &= ~CASTLE_BK;
    if (src == static_cast<int>(Square::a8) || tgt == static_cast<int>(Square::a8)) board.castling_rights &= ~CASTLE_BQ;

    // 9. Update Global Occupancies
    for (int i = 0; i < 3; ++i) board.occupancies[i] = 0ULL;
    for (int i = 0; i < 6; ++i) {
        board.occupancies[static_cast<int>(Color::White)] |= board.pieces[static_cast<int>(Color::White)][i];
        board.occupancies[static_cast<int>(Color::Black)] |= board.pieces[static_cast<int>(Color::Black)][i];
    }
    board.occupancies[static_cast<int>(Color::Both)] = board.occupancies[static_cast<int>(Color::White)] | board.occupancies[static_cast<int>(Color::Black)];

    // 10. Switch turns
    board.side_to_move = (us == static_cast<int>(Color::White)) ? Color::Black : Color::White;

    // 11. Is the King in check after the move?
    int king_sq = __builtin_ctzll(board.pieces[us][KING]);
    if (is_square_attacked(static_cast<Square>(king_sq), (us == static_cast<int>(Color::White)) ? Color::Black : Color::White, board)) {
        return false; // Illegal move!
    }

    return true; // Legal move!
}

} // namespace MoveGen
