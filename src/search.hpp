#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "evaluate.hpp"
#include "tt.hpp"
#include <iostream>
#include <chrono>

namespace Search {

    const int INFINITY_SCORE = 30000;
    inline long long nodes_searched = 0;

    // --- TIME MANAGEMENT ---
    inline long long start_time_ms = 0;
    inline long long allocated_time_ms = 0; // Hard limit
    inline long long soft_time_limit = 0;   // Soft limit
    inline bool time_is_up = false;

    inline long long get_time_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    inline void check_time() {
        if (time_is_up) return;
        if (get_time_ms() - start_time_ms >= allocated_time_ms) {
            time_is_up = true;
        }
    }

    // --- KILLER MOVES ---
    inline MoveGen::Move killer_moves[64][2];

    // --- MOVE ORDERING ---
    inline int get_piece_value(const Board& board, Color color, Square sq) {
        uint64_t bitboard_mask = 1ULL << static_cast<int>(sq);
        int c = static_cast<int>(color);
        if (board.pieces[c][PAWN]   & bitboard_mask) return Eval::PawnValue;
        if (board.pieces[c][KNIGHT] & bitboard_mask) return Eval::KnightValue;
        if (board.pieces[c][BISHOP] & bitboard_mask) return Eval::BishopValue;
        if (board.pieces[c][ROOK]   & bitboard_mask) return Eval::RookValue;
        if (board.pieces[c][QUEEN]  & bitboard_mask) return Eval::QueenValue;
        return 0;
    }

    inline int score_move(const Board& board, const MoveGen::Move& move, const MoveGen::Move* tt_move = nullptr, int ply = 0) {
        if (tt_move && move.source == tt_move->source && move.target == tt_move->target && move.promoted_piece == tt_move->promoted_piece) {
            return 2000000;
        }

        if (move.is_capture) {
            Color us = board.side_to_move;
            Color them = (us == Color::White) ? Color::Black : Color::White;
            int attacker_val = get_piece_value(board, us, move.source);
            int victim_val   = get_piece_value(board, them, move.target);
            return 1000000 + (victim_val * 10) - attacker_val;
        }
        
        if (move.promoted_piece != -1) return 900000;

        if (ply < 64) {
            if (move.source == killer_moves[ply][0].source && move.target == killer_moves[ply][0].target) return 800000;
            if (move.source == killer_moves[ply][1].source && move.target == killer_moves[ply][1].target) return 700000;
        }

        return 0; 
    }

    inline void sort_moves(const Board& board, MoveGen::MoveList& moves, const MoveGen::Move* tt_move = nullptr, int ply = 0) {
        int scores[256];
        for (int i = 0; i < moves.count; ++i) {
            scores[i] = score_move(board, moves.moves[i], tt_move, ply);
        }
        for (int i = 1; i < moves.count; ++i) {
            int key_score = scores[i];
            MoveGen::Move key_move = moves.moves[i];
            int j = i - 1;
            while (j >= 0 && scores[j] < key_score) {
                scores[j + 1] = scores[j];
                moves.moves[j + 1] = moves.moves[j];
                j = j - 1;
            }
            scores[j + 1] = key_score;
            moves.moves[j + 1] = key_move;
        }
    }

    // --- QUIESCENCE SEARCH ---
    inline int quiescence(Board& board, int alpha, int beta) {
        nodes_searched++;
        if ((nodes_searched & 2047) == 0) check_time();
        if (time_is_up) return 0;

        int stand_pat = Eval::evaluate(board);
        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) alpha = stand_pat;

        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);
        sort_moves(board, moves); 

        for (int i = 0; i < moves.count; ++i) {
            MoveGen::Move move = moves.moves[i];
            if (move.is_capture) {
                Board copy = board;
                if (MoveGen::make_move(copy, move)) {
                    int score = -quiescence(copy, -beta, -alpha);
                    if (score >= beta) return beta;
                    if (score > alpha) alpha = score;
                }
            }
        }
        return alpha;
    }

    // --- NEGAMAX WITH PVS, LMR, NULL MOVE, & CHECK EXTENSIONS ---
    inline int negamax(Board& board, int depth, int alpha, int beta, int ply, bool allow_null = true) {
        nodes_searched++;
        if ((nodes_searched & 2047) == 0) check_time();
        if (time_is_up) return 0;

        if (depth <= 0) return quiescence(board, alpha, beta);

        int original_alpha = alpha;
        uint64_t hash = TT::compute_hash(board);

        // 1. PROBE TRANSPOSITION TABLE
        MoveGen::Move tt_move{};
        bool has_tt_move = false;
        int tt_score;
        if (TT::probe(hash, depth, alpha, beta, ply, tt_score, tt_move, has_tt_move)) {
            return tt_score; 
        }

        int us = static_cast<int>(board.side_to_move);
        Color enemy = (board.side_to_move == Color::White) ? Color::Black : Color::White;
        int king_sq = __builtin_ctzll(board.pieces[us][KING]);
        bool in_check = MoveGen::is_square_attacked(static_cast<Square>(king_sq), enemy, board);

        // 2. NULL MOVE PRUNING
        if (allow_null && depth >= 3 && !in_check && ply > 0) {
            Board nmp_board = board;
            nmp_board.side_to_move = enemy;
            nmp_board.en_passant_square = -1;
            int nmp_score = -negamax(nmp_board, depth - 1 - 2, -beta, -beta + 1, ply + 1, false);
            if (nmp_score >= beta) return beta;
        }

        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);
        sort_moves(board, moves, has_tt_move ? &tt_move : nullptr, ply); 

        int legal_moves_played = 0;
        int max_score = -INFINITY_SCORE;
        MoveGen::Move best_move_this_node{}; 

        if (moves.count > 0) best_move_this_node = moves.moves[0]; 

        // 3. NORMAL SEARCH LOOP (WITH PVS & LMR)
        for (int i = 0; i < moves.count; ++i) {
            Board copy = board;
            if (MoveGen::make_move(copy, moves.moves[i])) {
                legal_moves_played++;
                
                // CHECK EXTENSIONS: Does this move put the opponent in check?
                int enemy_king_sq_new = __builtin_ctzll(copy.pieces[static_cast<int>(enemy)][KING]);
                bool gives_check = MoveGen::is_square_attacked(static_cast<Square>(enemy_king_sq_new), board.side_to_move, copy);
                int extension = gives_check ? 1 : 0;

                bool is_quiet = !moves.moves[i].is_capture && moves.moves[i].promoted_piece == -1;

                int score;
                // PRINCIPAL VARIATION SEARCH
                if (legal_moves_played == 1) {
                    // First move gets full window
                    score = -negamax(copy, depth - 1 + extension, -beta, -alpha, ply + 1, true);
                } else {
                    // LATE MOVE REDUCTION (LMR)
                    int R = 0;
                    if (legal_moves_played >= 4 && depth >= 3 && is_quiet && !in_check && !gives_check) {
                        R = (legal_moves_played > 6) ? 2 : 1;
                    }

                    // Zero-window search (assume it fails)
                    score = -negamax(copy, depth - 1 + extension - R, -alpha - 1, -alpha, ply + 1, true);
                    
                    // If it failed high, re-search with full window and no reduction
                    if (score > alpha && score < beta) {
                        score = -negamax(copy, depth - 1 + extension, -beta, -alpha, ply + 1, true);
                    }
                }

                if (time_is_up) return 0;

                if (score > max_score) {
                    max_score = score;
                    best_move_this_node = moves.moves[i];
                }
                if (max_score > alpha) alpha = max_score;
                
                if (alpha >= beta) {
                    // SAVE KILLER MOVES
                    if (is_quiet && ply < 64) {
                        killer_moves[ply][1] = killer_moves[ply][0];
                        killer_moves[ply][0] = moves.moves[i];
                    }
                    break; 
                }
            }
        }

        if (legal_moves_played == 0) {
            if (in_check) max_score = -INFINITY_SCORE + ply; // Checkmate
            else max_score = 0; // Stalemate
        }

        // 4. STORE IN TRANSPOSITION TABLE
        int flag = TT::UPPERBOUND;
        if (max_score >= beta) flag = TT::LOWERBOUND;
        else if (max_score > original_alpha) flag = TT::EXACT;

        TT::store(hash, depth, max_score, flag, best_move_this_node, ply);

        return max_score;
    }

    // --- ITERATIVE DEEPENING & SMART TIME MANAGEMENT ---
    inline MoveGen::Move search_best_move(Board& board, long long base_time_ms) {
        start_time_ms = get_time_ms();
        soft_time_limit = base_time_ms;
        allocated_time_ms = base_time_ms * 3; // Hard limit is 3x the soft limit
        time_is_up = false;
        nodes_searched = 0;

        for (int i = 0; i < 64; i++) {
            killer_moves[i][0] = MoveGen::Move{};
            killer_moves[i][1] = MoveGen::Move{};
        }

        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);
        sort_moves(board, moves);

        MoveGen::Move best_move_overall{};
        if (moves.count > 0) best_move_overall = moves.moves[0];
        
        MoveGen::Move previous_best_move = best_move_overall;

        for (int depth = 1; depth <= 64; ++depth) {
            int best_score_this_depth = -INFINITY_SCORE;
            MoveGen::Move best_move_this_depth = moves.moves[0];

            int alpha = -INFINITY_SCORE;
            int beta = INFINITY_SCORE;
            int legal_moves_played = 0;

            for (int i = 0; i < moves.count; ++i) {
                Board copy = board;
                if (MoveGen::make_move(copy, moves.moves[i])) {
                    legal_moves_played++;
                    
                    int enemy_king_sq_new = __builtin_ctzll(copy.pieces[static_cast<int>((board.side_to_move == Color::White) ? Color::Black : Color::White)][KING]);
                    bool gives_check = MoveGen::is_square_attacked(static_cast<Square>(enemy_king_sq_new), board.side_to_move, copy);
                    int extension = gives_check ? 1 : 0;

                    int score;
                    if (legal_moves_played == 1) {
                        score = -negamax(copy, depth - 1 + extension, -beta, -alpha, 1, true);
                    } else {
                        score = -negamax(copy, depth - 1 + extension, -alpha - 1, -alpha, 1, true);
                        if (score > alpha && score < beta) {
                            score = -negamax(copy, depth - 1 + extension, -beta, -alpha, 1, true);
                        }
                    }

                    if (time_is_up) break;

                    if (score > best_score_this_depth) {
                        best_score_this_depth = score;
                        best_move_this_depth = moves.moves[i];
                    }
                    if (best_score_this_depth > alpha) {
                        alpha = best_score_this_depth;
                    }
                }
            }

            if (time_is_up) break;

            best_move_overall = best_move_this_depth;
            long long time_spent = get_time_ms() - start_time_ms;

            std::cout << "info depth " << depth
                      << " score cp " << best_score_this_depth
                      << " nodes " << nodes_searched
                      << " time " << time_spent
                      << " pv " << best_move_overall.to_string() << "\n";

            // If the best move changed at a high depth, give the engine more time to think!
            if (depth > 1 && (best_move_overall.source != previous_best_move.source || best_move_overall.target != previous_best_move.target)) {
                soft_time_limit += base_time_ms / 2;
                if (soft_time_limit > allocated_time_ms) soft_time_limit = allocated_time_ms;
            }
            previous_best_move = best_move_overall;

            // Stop searching if we found a mate
            if (best_score_this_depth > 20000 || best_score_this_depth < -20000) break;

            // Stop searching if we've used up our soft time limit
            if (time_spent > soft_time_limit * 0.6) {
                break;
            }
        }

        return best_move_overall;
    }

} // namespace Search
