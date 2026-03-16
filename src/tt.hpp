#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include <random>
#include <cstdint>
#include <vector>

namespace TT {

    // --- ZOBRIST KEYS ---
    inline uint64_t piece_keys[2][6][64];
    inline uint64_t side_key;
    inline uint64_t ep_keys[65]; 

    // Generate random 64-bit numbers for every possible game state
    inline void init_zobrist() {
        std::mt19937_64 rng(12345); // Fixed seed
        for (int c = 0; c < 2; c++)
            for (int p = 0; p < 6; p++)
                for (int s = 0; s < 64; s++)
                    piece_keys[c][p][s] = rng();
        
        side_key = rng();
        for (int i = 0; i < 65; i++) ep_keys[i] = rng();
    }

    // Build the unique 64-bit ID from scratch
    inline uint64_t compute_hash(const Board& board) {
        uint64_t hash = 0;
        for (int c = 0; c < 2; c++) {
            for (int p = 0; p < 6; p++) {
                uint64_t bb = board.pieces[c][p];
                while (bb) {
                    int sq = __builtin_ctzll(bb);
                    hash ^= piece_keys[c][p][sq];
                    bb &= (bb - 1);
                }
            }
        }
        if (board.side_to_move == Color::Black) hash ^= side_key;
        
        int ep = board.en_passant_square;
        if (ep >= 0 && ep < 64) hash ^= ep_keys[ep];
        else hash ^= ep_keys[64];
        
        return hash;
    }

    // --- TRANSPOSITION TABLE ---
    enum Flag { EXACT, LOWERBOUND, UPPERBOUND };

    struct Entry {
        uint64_t key = 0;
        int score = 0;
        int depth = -1;
        int flag = 0;
        MoveGen::Move best_move;
    };

    // Size: ~1 million entries (approx 24 MB of RAM)
    const int TT_SIZE = 1048576; 
    inline std::vector<Entry> table;

    inline void init_tt() {
        table.resize(TT_SIZE);
    }

    inline void clear_tt() {
        for (auto& entry : table) {
            entry.key = 0;
            entry.depth = -1;
        }
    }

    // Save a position to memory
    inline void store(uint64_t key, int depth, int score, int flag, MoveGen::Move best_move, int ply) {
        // Mate Score trick: Convert distance-to-mate to absolute distance from root
        if (score > 20000) score += ply;
        if (score < -20000) score -= ply;

        int index = key % TT_SIZE;
        table[index].key = key;
        table[index].depth = depth;
        table[index].score = score;
        table[index].flag = flag;
        table[index].best_move = best_move;
    }

    // Check if we have seen this position before
    inline bool probe(uint64_t key, int depth, int alpha, int beta, int ply, int& return_score, MoveGen::Move& best_move, bool& has_tt_move) {
        int index = key % TT_SIZE;
        Entry& entry = table[index];

        if (entry.key == key) {
            best_move = entry.best_move;
            has_tt_move = true; // We remember the best move from last time!

            if (entry.depth >= depth) {
                int score = entry.score;
                // Reverse the mate score trick
                if (score > 20000) score -= ply;
                if (score < -20000) score += ply;

                if (entry.flag == EXACT) { return_score = score; return true; }
                if (entry.flag == LOWERBOUND && score >= beta) { return_score = score; return true; }
                if (entry.flag == UPPERBOUND && score <= alpha) { return_score = score; return true; }
            }
        }
        return false;
    }
}
