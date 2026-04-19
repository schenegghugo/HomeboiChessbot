#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "nnue.hpp"
#include "tt.hpp" // <-- ADDED TT
#include <iostream>
#include <chrono>
#include <algorithm>

namespace Search {
    const int INFINITY_SCORE = 30000;
    const int MAX_PLY = 100; 
    
    inline long long nodes_searched = 0;
    inline long long start_time_ms = 0;
    inline long long allocated_time_ms = 0; 
    inline long long soft_time_limit = 0;   
    inline bool time_is_up = false;

    inline Move killer_moves[MAX_PLY][2];
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

    // Checks if a square is attacked
    inline bool is_square_attacked(const Board& board, int sq, int attacker_side) {
        U64 blockers = board.colors[WHITE] | board.colors[BLACK];
        int p_off = (attacker_side == WHITE) ? 0 : 6;
        
        U64 pawns = board.pieces[wP + p_off];
        U64 pawn_attacks = (attacker_side == WHITE) ? 
            (((1ULL << sq) >> 7) & 0xfefefefefefefefeULL) | (((1ULL << sq) >> 9) & 0x7f7f7f7f7f7f7f7fULL) :
            (((1ULL << sq) << 9) & 0xfefefefefefefefeULL) | (((1ULL << sq) << 7) & 0x7f7f7f7f7f7f7f7fULL);
        if (pawn_attacks & pawns) return true;

        if (KnightAttacks[sq] & board.pieces[wN + p_off]) return true;
        if (KingAttacks[sq] & board.pieces[wK + p_off]) return true;
        if (get_bishop_attacks(sq, blockers) & (board.pieces[wB + p_off] | board.pieces[wQ + p_off])) return true;
        if (get_rook_attacks(sq, blockers) & (board.pieces[wR + p_off] | board.pieces[wQ + p_off])) return true;
        
        return false;
    }

    // --- MVV-LVA Move Scoring ---
    inline int score_move(const Board& board, const Move& move, const Move& tt_move = Move(), int ply = 0) {
        if (tt_move.piece != EMPTY && move == tt_move) {
            return 2000000;
        }
        if (move.captured != EMPTY) {
            static const int piece_vals[12] = { 100, 300, 300, 500, 900, 20000, 100, 300, 300, 500, 900, 20000 };
            int victim_val = piece_vals[move.captured];
            int attacker_val = piece_vals[move.piece];
            return 1000000 + (victim_val * 10) - attacker_val;
        }
        if (move.promoted != EMPTY) return 900000;
        
        if (ply < MAX_PLY) {
            if (move == killer_moves[ply][0]) return 800000;
            if (move == killer_moves[ply][1]) return 700000;
        }
        
        int side = board.side_to_move;
        return std::min(history_moves[side][move.from][move.to], 500000);
    }

    inline Move pick_next_move(MoveList& moves, int* scores, int start_idx) {
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

    inline void sort_moves_root(const Board& board, MoveList& moves, const Move& tt_move = Move()) {
        int scores[256];
        for (int i = 0; i < moves.count; ++i) scores[i] = score_move(board, moves.moves[i], tt_move, 0);
        for (int i = 0; i < moves.count; ++i) pick_next_move(moves, scores, i);
    }

    // --- Quiescence Search ---
    inline int quiescence(Board board, int alpha, int beta, int ply) {
        int us = board.side_to_move;
        int enemy = us ^ 1;

        int my_king_sq = (us == WHITE) ? board.white_king_sq : board.black_king_sq;
        bool in_check = is_square_attacked(board, my_king_sq, enemy);

        nodes_searched++;
        if ((nodes_searched & 2047) == 0) check_time();
        if (time_is_up) return 0;
        
        if (!in_check) {
            int stand_pat = evaluate_nnue(ply, us);
            if (stand_pat >= beta) return beta;
            if (stand_pat > alpha) alpha = stand_pat;
        }
        
        MoveList moves = generate_pseudo_legal_moves(board);

        int scores[256];
        for (int i = 0; i < moves.count; ++i) scores[i] = score_move(board, moves.moves[i], Move(), ply);
        
        int legal_moves_played = 0;
        for (int i = 0; i < moves.count; ++i) {
            Move move = pick_next_move(moves, scores, i); 
            
            if (!in_check && move.captured == EMPTY && move.promoted == EMPTY) continue;

            Board next_board = board;
            next_board.make_move(move);

            if (is_square_attacked(next_board, my_king_sq, enemy)) continue;
            legal_moves_played++;
            
            update_accumulator(ply + 1, move, next_board);

            int score = -quiescence(next_board, -beta, -alpha, ply + 1);
            
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
        
        if (in_check && legal_moves_played == 0) return -INFINITY_SCORE + ply;

        return alpha;
    }

    // --- Main Negamax Search ---
    inline int negamax(Board board, int depth, int alpha, int beta, int ply, bool allow_null = true) {
        if (ply >= MAX_PLY) return evaluate_nnue(ply, board.side_to_move);

        int us = board.side_to_move;
        int enemy = us ^ 1;

        int alphaOrig = alpha; // <-- TT: Save original alpha for bounds check
        alpha = std::max(alpha, -INFINITY_SCORE + ply);
        beta = std::min(beta, INFINITY_SCORE - ply - 1);
        if (alpha >= beta) return alpha;

        // --- 1. TT PROBE ---
        uint16_t tt_move_compressed = 0;
        TTEntry& tte = TT[board.hash % TT_SIZE];
        
        if (tte.key == board.hash) {
            tt_move_compressed = tte.move;
            if (tte.depth >= depth) {
                if (tte.flag == TT_EXACT) return tte.score;
                if (tte.flag == TT_ALPHA && tte.score <= alpha) return alpha;
                if (tte.flag == TT_BETA && tte.score >= beta) return beta;
            }
        }

        // TODO: Draw Detection (Requires Repetition History Array)

        nodes_searched++;
        if ((nodes_searched & 2047) == 0) check_time();
        if (time_is_up) return 0;
        
        if (depth <= 0) return quiescence(board, alpha, beta, ply);

        int my_king_sq = (us == WHITE) ? board.white_king_sq : board.black_king_sq;
        bool in_check = is_square_attacked(board, my_king_sq, enemy);
        int extension = in_check ? 1 : 0;

        // Null Move Pruning (NNUE Adjusted)
        bool has_major_pieces = (board.pieces[wN + us*6] | board.pieces[wB + us*6] | board.pieces[wR + us*6] | board.pieces[wQ + us*6]) != 0;
        if (allow_null && depth >= 3 && !in_check && ply > 0 && has_major_pieces) {
            Board next_board = board;
            next_board.side_to_move ^= 1; 
            next_board.ep_square = EMPTY;
            next_board.hash ^= Zobrist::side_key; // Keep Hash Updated in NMP

            accumulators[ply + 1] = accumulators[ply];
            
            int nmp_score = -negamax(next_board, depth - 1 - 2, -beta, -beta + 1, ply + 1, false);
            
            if (time_is_up) return 0;
            if (nmp_score >= beta) return beta; 
        }

        MoveList moves = generate_pseudo_legal_moves(board);
        
        // --- 2. TT MOVE EXTRACTION FOR ORDERING ---
        Move tt_move;
        tt_move.piece = EMPTY;
        if (tt_move_compressed != 0) {
            for (int i = 0; i < moves.count; ++i) {
                if (compress_move(moves.moves[i]) == tt_move_compressed) {
                    tt_move = moves.moves[i];
                    break;
                }
            }
        }

        int scores[256];
        for (int i = 0; i < moves.count; ++i) {
            scores[i] = score_move(board, moves.moves[i], tt_move, ply);
        }
        
        int legal_moves_played = 0;
        int max_score = -INFINITY_SCORE;
        Move best_move_this_node;
        best_move_this_node.piece = EMPTY;

        for (int i = 0; i < moves.count; ++i) {
            Move move = pick_next_move(moves, scores, i);
            
            Board next_board = board;
            next_board.make_move(move);

            int king_after = (us == WHITE) ? next_board.white_king_sq : next_board.black_king_sq;
            if (is_square_attacked(next_board, king_after, enemy)) continue;

            legal_moves_played++;
            
            update_accumulator(ply + 1, move, next_board);

            bool is_quiet = (move.captured == EMPTY && move.promoted == EMPTY);
            int score;
            int new_depth = depth - 1 + extension;

            if (legal_moves_played == 1) {
                score = -negamax(next_board, new_depth, -beta, -alpha, ply + 1, true);
            } else {
                bool needs_full_search = true;
                bool is_killer = (ply < MAX_PLY) && (move == killer_moves[ply][0] || move == killer_moves[ply][1]);

                if (new_depth >= 2 && legal_moves_played >= 4 && is_quiet && !in_check && !is_killer) {
                    int R = 1;
                    if (legal_moves_played > 6) R++;
                    if (new_depth >= 4) R++; 
                    
                    score = -negamax(next_board, new_depth - R, -alpha - 1, -alpha, ply + 1, true);
                    if (score <= alpha) needs_full_search = false; 
                }

                if (needs_full_search) {
                    score = -negamax(next_board, new_depth, -alpha - 1, -alpha, ply + 1, true);
                    if (score > alpha && score < beta) {
                        score = -negamax(next_board, new_depth, -beta, -alpha, ply + 1, true);
                    }
                }
            }
            
            if (time_is_up) return 0;

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
                    int& hist = history_moves[us][move.from][move.to];
                    hist += bonus - static_cast<int>((static_cast<long long>(hist) * bonus) / 500000);
                }
                break; 
            }
        }
        
        if (legal_moves_played == 0) {
            if (in_check) return -INFINITY_SCORE + ply; 
            else return 0; 
        }
        
        // --- 3. TT STORE ---
        int flag = TT_EXACT;
        if (max_score <= alphaOrig) flag = TT_ALPHA;
        else if (max_score >= beta) flag = TT_BETA;
        
        tt_save(board.hash, depth, max_score, flag, best_move_this_node);

        return max_score;
    }

    // --- Iterative Deepening Framework ---
    inline Move search_best_move(Board board, long long base_time_ms) {
        start_time_ms = get_time_ms();
        soft_time_limit = base_time_ms;
        allocated_time_ms = base_time_ms * 3;
        time_is_up = false;
        nodes_searched = 0;
        
        for (int i = 0; i < MAX_PLY; i++) {
            killer_moves[i][0] = Move();
            killer_moves[i][1] = Move();
        }
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 64; j++) {
                for (int k = 0; k < 64; k++) history_moves[i][j][k] /= 2;
            }
        }

        MoveList moves = generate_pseudo_legal_moves(board);
        if (moves.count == 0) return Move(); 

        Move best_move_overall; 
        Move previous_best_move;

        refresh_accumulator(board, 0);

        for (int depth = 1; depth <= 64; ++depth) {
            int best_score_this_depth = -INFINITY_SCORE;
            Move best_move_this_depth;
            int alpha = -INFINITY_SCORE;
            int beta = INFINITY_SCORE;
            int legal_moves_played = 0;
            
            sort_moves_root(board, moves, previous_best_move);

            for (int i = 0; i < moves.count; ++i) {
                Move move = moves.moves[i];
                int us = board.side_to_move;
                int enemy = us ^ 1;

                Board next_board = board;
                next_board.make_move(move);

                int king_after = (us == WHITE) ? next_board.white_king_sq : next_board.black_king_sq;
                if (is_square_attacked(next_board, king_after, enemy)) continue;
                
                legal_moves_played++;
                
                update_accumulator(1, move, next_board);

                int score;
                int new_depth = depth - 1;
                
                if (legal_moves_played == 1) {
                    score = -negamax(next_board, new_depth, -beta, -alpha, 1, true);
                } else {
                    score = -negamax(next_board, new_depth, -alpha - 1, -alpha, 1, true);
                    if (score > alpha && score < beta) {
                        score = -negamax(next_board, new_depth, -beta, -alpha, 1, true);
                    }
                }
                
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
            
            std::string from_str = std::string{char('a' + (best_move_overall.from % 8)), char('1' + (best_move_overall.from / 8))};
            std::string to_str   = std::string{char('a' + (best_move_overall.to % 8)),   char('1' + (best_move_overall.to / 8))};
            
            std::cout << "info depth " << depth << " score cp " << best_score_this_depth 
                      << " nodes " << nodes_searched << " time " << time_spent 
                      << " pv " << from_str << to_str << "\n";
            
            if (depth > 1 && best_move_overall != previous_best_move) {
                soft_time_limit += base_time_ms / 2;
                if (soft_time_limit > allocated_time_ms) soft_time_limit = allocated_time_ms;
            }
            
            previous_best_move = best_move_overall;
            if (best_score_this_depth > 29000) { if (depth >= 8) break; }
            if (time_spent > soft_time_limit * 0.6) break; 
        }
        return best_move_overall;
    }
}
