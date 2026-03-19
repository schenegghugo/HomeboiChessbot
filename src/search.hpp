#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "evaluate.hpp"
#include "tt.hpp"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <algorithm>

namespace Search {
    const int INFINITY_SCORE = 30000;
    const int MAX_PLY = 100; 
    
    inline long long nodes_searched = 0;
    inline long long start_time_ms = 0;
    inline long long allocated_time_ms = 0; 
    inline long long soft_time_limit = 0;   
    inline bool time_is_up = false;

    inline MoveGen::Move killer_moves[MAX_PLY][2] = {{0}};
    inline int history_moves[2][64][64] = {{{0}}};

    inline long long get_time_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    inline void check_time() {
        if (time_is_up) return;
        if (get_time_ms() - start_time_ms >= allocated_time_ms) time_is_up = true;
    }

    inline bool is_draw(const Board& board, uint64_t current_hash) {
        if (board.half_move_clock >= 100) return true;
        int limit = std::max(0, board.history_ply - board.half_move_clock);
        for (int i = board.history_ply - 2; i >= limit; i -= 2) {
            if (board.repetition_history[i] == current_hash) return true;
        }
        return false;
    }

    // --- NEW O(1) SCORE MOVE ---
    inline int score_move(const Board& board, MoveGen::Move move, MoveGen::Move tt_move = 0, int ply = 0) {
        if (tt_move != 0 && move == tt_move) {
            return 2000000;
        }
        if (MoveGen::is_capture(move)) {
            // Fast lookup array: P=0, N=1, B=2, R=3, Q=4, K=5, EMPTY=6
            static const int piece_vals[7] = {
                Eval::PawnValue, Eval::KnightValue, Eval::BishopValue, 
                Eval::RookValue, Eval::QueenValue, 20000, 0
            };
            
            // --- O(1) Piece Identifiers ---
            int attacker_piece = board.piece_on[MoveGen::get_source(move)];
            int victim_piece   = board.piece_on[MoveGen::get_target(move)];
            
            // Handle En Passant: The target square is EMPTY, but we captured a Pawn
            int victim_val = (victim_piece == EMPTY) ? Eval::PawnValue : piece_vals[victim_piece];
            int attacker_val = piece_vals[attacker_piece];
            
            return 1000000 + (victim_val * 10) - attacker_val;
        }
        if (MoveGen::is_promotion(move)) return 900000;
        
        if (ply < MAX_PLY) {
            if (move == killer_moves[ply][0]) return 800000;
            if (move == killer_moves[ply][1]) return 700000;
        }
        
        int side = static_cast<int>(board.side_to_move);
        return std::min(history_moves[side][MoveGen::get_source(move)][MoveGen::get_target(move)], 500000);
    }

    inline MoveGen::Move pick_next_move(MoveGen::MoveList& moves, int* scores, int start_idx) {
        int best_score = -INFINITY_SCORE * 2;
        int best_idx = start_idx;
        for (int j = start_idx; j < moves.count; ++j) {
            if (scores[j] > best_score) {
                best_score = scores[j];
                best_idx = j;
            }
        }
        std::swap(moves.moves[start_idx], moves.moves[best_idx]);
        std::swap(scores[start_idx], scores[best_idx]);
        return moves.moves[start_idx];
    }

    inline void sort_moves_root(const Board& board, MoveGen::MoveList& moves, MoveGen::Move tt_move = 0) {
        int scores[256];
        for (int i = 0; i < moves.count; ++i) scores[i] = score_move(board, moves.moves[i], tt_move, 0);
        for (int i = 0; i < moves.count; ++i) pick_next_move(moves, scores, i);
    }

    inline int quiescence(Board& board, int alpha, int beta) {
        int us = static_cast<int>(board.side_to_move);
        Color enemy = (board.side_to_move == Color::White) ? Color::Black : Color::White;

        uint64_t my_king_bb = board.pieces[us][KING];
        if (my_king_bb == 0) return -INFINITY_SCORE + 100; 
        int my_king_sq = __builtin_ctzll(my_king_bb);
        bool in_check = MoveGen::is_square_attacked(static_cast<Square>(my_king_sq), enemy, board);

        uint64_t enemy_king_bb = board.pieces[static_cast<int>(enemy)][KING];
        if (enemy_king_bb == 0) return INFINITY_SCORE; 
        int enemy_king_sq = __builtin_ctzll(enemy_king_bb);
        if (MoveGen::is_square_attacked(static_cast<Square>(enemy_king_sq), static_cast<Color>(us), board)) {
            return INFINITY_SCORE;
        }

        nodes_searched++;
        if ((nodes_searched & 2047) == 0) check_time();
        if (time_is_up) return 0;
        
        if (!in_check) {
            int stand_pat = Eval::evaluate(board);
            if (stand_pat >= beta) return beta;
            if (stand_pat > alpha) alpha = stand_pat;
        }
        
        MoveGen::MoveList moves;
        if (in_check) {
            moves = MoveGen::generate_pseudo_legal_moves(board);
        } else {
            moves = MoveGen::generate_captures(board);
        }

        int scores[256];
        for (int i = 0; i < moves.count; ++i) scores[i] = score_move(board, moves.moves[i], 0, 0);
        
        int legal_moves_played = 0;
        for (int i = 0; i < moves.count; ++i) {
            MoveGen::Move move = pick_next_move(moves, scores, i); 
            
            if (MoveGen::make_move(board, move)) {
                legal_moves_played++;
                int score = -quiescence(board, -beta, -alpha);
                MoveGen::unmake_move(board, move);
                
                if (score >= beta) return beta;
                if (score > alpha) alpha = score;
            }
        }
        
        if (in_check && legal_moves_played == 0) {
            return -INFINITY_SCORE + 100;
        }

        return alpha;
    }

    inline int negamax(Board& board, int depth, int alpha, int beta, int ply, bool allow_null = true) {
        if (ply >= MAX_PLY) return Eval::evaluate(board);

        int us = static_cast<int>(board.side_to_move);
        Color enemy = (board.side_to_move == Color::White) ? Color::Black : Color::White;

        uint64_t enemy_king_bb = board.pieces[static_cast<int>(enemy)][KING];
        if (enemy_king_bb == 0) return INFINITY_SCORE - ply;
        int enemy_king_sq = __builtin_ctzll(enemy_king_bb);
        if (MoveGen::is_square_attacked(static_cast<Square>(enemy_king_sq), static_cast<Color>(us), board)) {
            return INFINITY_SCORE - ply; 
        }

        alpha = std::max(alpha, -INFINITY_SCORE + ply);
        beta = std::min(beta, INFINITY_SCORE - ply - 1);
        if (alpha >= beta) return alpha;

        uint64_t hash = board.hash; 
        board.repetition_history[board.history_ply] = hash; 
        
        if (ply > 0 && is_draw(board, hash)) return 0; 
        
        nodes_searched++;
        if ((nodes_searched & 2047) == 0) check_time();
        if (time_is_up) return 0;
        
        if (depth <= 0) return quiescence(board, alpha, beta);

        int original_alpha = alpha;
        MoveGen::Move tt_move = 0;
        bool has_tt_move = false;
        int tt_score;
        
        if (TT::probe(hash, depth, alpha, beta, ply, tt_score, tt_move, has_tt_move)) {
            if (tt_score > INFINITY_SCORE - 1000) tt_score -= ply;
            else if (tt_score < -INFINITY_SCORE + 1000) tt_score += ply;
            return tt_score;
        }

        int king_sq = __builtin_ctzll(board.pieces[us][KING]);
        bool in_check = MoveGen::is_square_attacked(static_cast<Square>(king_sq), enemy, board);

        int extension = in_check ? 1 : 0;

        if (depth <= 3 && !in_check && !has_tt_move && ply > 0) {
            int static_eval = Eval::evaluate(board); 
            int margin = depth * 150; 
            if (static_eval - margin >= beta) {
                return static_eval; 
            }
        }

        bool has_major_pieces = (board.pieces[us][KNIGHT] | board.pieces[us][BISHOP] | board.pieces[us][ROOK] | board.pieces[us][QUEEN]) != 0;
        if (allow_null && depth >= 3 && !in_check && ply > 0 && has_major_pieces) {
            Color old_side = board.side_to_move;
            int old_ep = board.en_passant_square;
            uint64_t old_hash = board.hash; 
            
            board.side_to_move = enemy;
            if (board.en_passant_square != -1) {
                board.hash ^= Zobrist::ep_keys[board.en_passant_square % 8]; 
                board.en_passant_square = -1;
            }
            board.hash ^= Zobrist::side_key;
            
            int nmp_score = -negamax(board, depth - 1 - 2, -beta, -beta + 1, ply + 1, false);
            
            board.side_to_move = old_side;
            board.en_passant_square = old_ep;
            board.hash = old_hash;
            
            if (time_is_up) return 0;
            if (nmp_score >= beta) return beta; 
        }

        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);
        
        int scores[256];
        for (int i = 0; i < moves.count; ++i) {
            scores[i] = score_move(board, moves.moves[i], has_tt_move ? tt_move : 0, ply);
        }
        
        int legal_moves_played = 0;
        int max_score = -INFINITY_SCORE;
        MoveGen::Move best_move_this_node = 0;

        for (int i = 0; i < moves.count; ++i) {
            MoveGen::Move move = pick_next_move(moves, scores, i);
            
            if (!MoveGen::make_move(board, move)) continue; 

            legal_moves_played++;
            
            bool is_quiet = !MoveGen::is_capture(move) && !MoveGen::is_promotion(move);
            int score;
            int new_depth = depth - 1 + extension;

            if (legal_moves_played == 1) {
                score = -negamax(board, new_depth, -beta, -alpha, ply + 1, true);
            } else {
                bool needs_full_search = true;
                bool is_killer = false;
                
                if (ply < MAX_PLY) {
                    is_killer = (move == killer_moves[ply][0] || move == killer_moves[ply][1]);
                }

                if (new_depth >= 2 && legal_moves_played >= 4 && is_quiet && !in_check && !is_killer) {
                    uint64_t opp_king_bb = board.pieces[static_cast<int>(enemy)][KING];
                    bool gives_check = (opp_king_bb) ? MoveGen::is_square_attacked(static_cast<Square>(__builtin_ctzll(opp_king_bb)), static_cast<Color>(us), board) : false;

                    if (!gives_check) {
                        int R = 1;
                        if (legal_moves_played > 6) R++;
                        if (new_depth >= 4) R++; 
                        
                        score = -negamax(board, new_depth - R, -alpha - 1, -alpha, ply + 1, true);
                        if (score <= alpha) needs_full_search = false; 
                    }
                }

                if (needs_full_search) {
                    score = -negamax(board, new_depth, -alpha - 1, -alpha, ply + 1, true);
                    if (score > alpha && score < beta) {
                        score = -negamax(board, new_depth, -beta, -alpha, ply + 1, true);
                    }
                }
            }
            
            MoveGen::unmake_move(board, move);

            if (time_is_up) return 0;

            if (score <= -INFINITY_SCORE + MAX_PLY) {
                legal_moves_played--; 
                continue; 
            }

            if (score > max_score) {
                max_score = score;
                best_move_this_node = move;
            }
            if (max_score > alpha) alpha = max_score;
            
            if (alpha >= beta) {
                if (is_quiet && ply < MAX_PLY) {
                    killer_moves[ply][1] = killer_moves[ply][0];
                    killer_moves[ply][0] = move;
                    
                    int bonus = depth * depth;
                    int src = MoveGen::get_source(move);
                    int dst = MoveGen::get_target(move);
                    int& hist = history_moves[us][src][dst];
                    
                    hist += bonus - static_cast<int>((static_cast<long long>(hist) * bonus) / 500000);
                }
                break; 
            }
        }
        
        if (legal_moves_played == 0) {
            if (in_check) return -INFINITY_SCORE + ply; 
            else return 0; 
        }
        
        int flag = TT::UPPERBOUND;
        if (max_score >= beta) flag = TT::LOWERBOUND;
        else if (max_score > original_alpha) flag = TT::EXACT;
        
        int store_score = max_score;
        if (store_score > INFINITY_SCORE - 1000) store_score += ply;
        else if (store_score < -INFINITY_SCORE + 1000) store_score -= ply;

        TT::store(hash, depth, store_score, flag, best_move_this_node, ply);
        return max_score;
    }

    inline MoveGen::Move search_best_move(Board& board, long long base_time_ms) {
        TT::current_age++; 
        start_time_ms = get_time_ms();
        soft_time_limit = base_time_ms;
        allocated_time_ms = base_time_ms * 3;
        time_is_up = false;
        nodes_searched = 0;
        
        for (int i = 0; i < MAX_PLY; i++) {
            killer_moves[i][0] = 0;
            killer_moves[i][1] = 0;
        }
        
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 64; k++) {
                    history_moves[i][j][k] /= 2;
                }
            }
        }

        int root_us = static_cast<int>(board.side_to_move);
        Color root_enemy = (board.side_to_move == Color::White) ? Color::Black : Color::White;
        uint64_t root_king_bb = board.pieces[root_us][KING];
        bool root_in_check = false;
        if (root_king_bb) {
            root_in_check = MoveGen::is_square_attacked(static_cast<Square>(__builtin_ctzll(root_king_bb)), root_enemy, board);
        }
        int root_extension = root_in_check ? 1 : 0;

        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);
        if (moves.count == 0) return 0; 

        MoveGen::Move best_move_overall = 0; 
        MoveGen::Move previous_best_move = 0;

        for (int depth = 1; depth <= 64; ++depth) {
            int best_score_this_depth = -INFINITY_SCORE;
            MoveGen::Move best_move_this_depth = 0;
            int alpha = -INFINITY_SCORE;
            int beta = INFINITY_SCORE;
            int legal_moves_played = 0;
            
            sort_moves_root(board, moves, previous_best_move);

            for (int i = 0; i < moves.count; ++i) {
                MoveGen::Move move = moves.moves[i];
                int us = static_cast<int>(board.side_to_move);
                Color enemy = (board.side_to_move == Color::White) ? Color::Black : Color::White;

                if (!MoveGen::make_move(board, move)) continue; 
                
                uint64_t current_king_bb = board.pieces[us][KING];
                if (current_king_bb == 0 || MoveGen::is_square_attacked(static_cast<Square>(__builtin_ctzll(current_king_bb)), enemy, board)) {
                    MoveGen::unmake_move(board, move);
                    continue; 
                }
                
                legal_moves_played++;
                int score;
                int new_depth = depth - 1 + root_extension;
                
                if (legal_moves_played == 1) {
                    score = -negamax(board, new_depth, -beta, -alpha, 1, true);
                } else {
                    score = -negamax(board, new_depth, -alpha - 1, -alpha, 1, true);
                    if (score > alpha && score < beta) {
                        score = -negamax(board, new_depth, -beta, -alpha, 1, true);
                    }
                }
                
                MoveGen::unmake_move(board, move);
                
                if (time_is_up) break;
                
                if (score > best_score_this_depth) {
                    best_score_this_depth = score;
                    best_move_this_depth = move;
                }
                if (best_score_this_depth > alpha) alpha = best_score_this_depth;
            }
            
            if (time_is_up) break;
            if (legal_moves_played == 0) break;
            
            best_move_overall = best_move_this_depth;
            long long time_spent = get_time_ms() - start_time_ms;
            
            std::cout << "info depth " << depth << " score cp " << best_score_this_depth 
                      << " nodes " << nodes_searched << " time " << time_spent 
                      << " pv " << MoveGen::move_to_string(best_move_overall) << "\n";
            
            if (depth > 1 && best_move_overall != previous_best_move) {
                soft_time_limit += base_time_ms / 2;
                if (soft_time_limit > allocated_time_ms) soft_time_limit = allocated_time_ms;
            }
            
            previous_best_move = best_move_overall;
            
            if (best_score_this_depth > 29000) {
                if (depth >= 8 || best_score_this_depth == INFINITY_SCORE - 1) break; 
            }
            
            if (time_spent > soft_time_limit * 0.6) break; 
        }
        return best_move_overall;
    }
}
