#pragma once
#include "board.hpp"
#include "tbprobe.h"
#include <string>
#include <iostream>

namespace Syzygy {
    
    // --- Initialize Fathom ---
    inline void init(const std::string& path) {
        if (tb_init(path.c_str())) {
            std::cout << "info string Syzygy loaded! Max pieces: " << TB_LARGEST << std::endl;
        } else {
            std::cout << "info string Failed to load Syzygy from: " << path << std::endl;
        }
    }

    // --- Probes WDL during the negamax search tree ---
    inline unsigned probe_wdl(const Board& board) {
        int piece_count = __builtin_popcountll(board.occupancies[static_cast<int>(Color::Both)]);
        if (piece_count > TB_LARGEST || piece_count == 0) return TB_RESULT_FAILED;

        uint64_t white = board.occupancies[static_cast<int>(Color::White)];
        uint64_t black = board.occupancies[static_cast<int>(Color::Black)];
        uint64_t kings   = board.pieces[0][KING] | board.pieces[1][KING];
        uint64_t queens  = board.pieces[0][QUEEN] | board.pieces[1][QUEEN];
        uint64_t rooks   = board.pieces[0][ROOK] | board.pieces[1][ROOK];
        uint64_t bishops = board.pieces[0][BISHOP] | board.pieces[1][BISHOP];
        uint64_t knights = board.pieces[0][KNIGHT] | board.pieces[1][KNIGHT];
        uint64_t pawns   = board.pieces[0][PAWN] | board.pieces[1][PAWN];
        
        unsigned ep = (board.en_passant_square == -1) ? 0 : board.en_passant_square;
        bool turn = (board.side_to_move == Color::White);

        return tb_probe_wdl(white, black, kings, queens, rooks, bishops, knights, pawns,
                            board.half_move_clock, board.castling_rights, ep, turn);
    }

    // --- Probes the exact best move at the root of the search ---
    inline unsigned probe_root(const Board& board) {
        int piece_count = __builtin_popcountll(board.occupancies[static_cast<int>(Color::Both)]);
        if (piece_count > TB_LARGEST || piece_count == 0) return TB_RESULT_FAILED;

        uint64_t white = board.occupancies[static_cast<int>(Color::White)];
        uint64_t black = board.occupancies[static_cast<int>(Color::Black)];
        uint64_t kings   = board.pieces[0][KING] | board.pieces[1][KING];
        uint64_t queens  = board.pieces[0][QUEEN] | board.pieces[1][QUEEN];
        uint64_t rooks   = board.pieces[0][ROOK] | board.pieces[1][ROOK];
        uint64_t bishops = board.pieces[0][BISHOP] | board.pieces[1][BISHOP];
        uint64_t knights = board.pieces[0][KNIGHT] | board.pieces[1][KNIGHT];
        uint64_t pawns   = board.pieces[0][PAWN] | board.pieces[1][PAWN];
        
        unsigned ep = (board.en_passant_square == -1) ? 0 : board.en_passant_square;
        bool turn = (board.side_to_move == Color::White);

        return tb_probe_root(white, black, kings, queens, rooks, bishops, knights, pawns,
                             board.half_move_clock, board.castling_rights, ep, turn, nullptr);
    }
}
