#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include <cstdint>

namespace TT {

    enum Flag {
        EXACT,
        LOWERBOUND,
        UPPERBOUND
    };

    // Exactly 16 bytes! Fits 4 times perfectly into a 64-byte CPU cache line.
    struct TTEntry {
        uint64_t hash;           // 8 bytes
        MoveGen::Move best_move; // 2 bytes 
        int16_t score;           // 2 bytes
        int8_t depth;            // 1 byte
        uint8_t flag;            // 1 byte
        uint8_t age;             // 1 byte
                                 // +1 byte compiler padding = 16 bytes
    };

    // --- Mate Score Constants ---
    const int MATE_VALUE = 30000;
    const int MATE_BOUND = 29000; 

    // 2^24 entries. Consumes exactly 256 MB.
    const int TT_SIZE = 16777216; 
    
    inline TTEntry* table = nullptr;
    inline uint8_t current_age = 0; 

    inline void init() {
        if (!table) {
            table = new TTEntry[TT_SIZE](); 

            // Loop through and touch the memory to prevent page-fault lag spikes
            for (int i = 0; i < TT_SIZE; ++i) {
                table[i].hash = 0;
            }
        }
    }

    inline void clear_tt() {
        if (table) {
            for (int i = 0; i < TT_SIZE; ++i) {
                table[i].hash = 0;
                table[i].best_move = 0; // 0 represents our "null" move now
                table[i].depth = 0;
                table[i].score = 0;
                table[i].flag = 0;
                table[i].age = 0;
            }
        }
    }

    inline uint64_t compute_hash(Board& board) {
        return board.hash; 
    }

    inline void store(uint64_t hash, int depth, int score, int flag, MoveGen::Move best_move, int /*ply*/) {
        int index = hash & (TT_SIZE - 1); 

        // If we already evaluated this EXACT position to a better/equal depth on this turn, skip it.
        if (table[index].hash == hash && table[index].age == current_age && table[index].depth > depth) {
            return; 
        }

        table[index].hash = hash;
        table[index].best_move = best_move;
        
        // Store the score exactly as search.hpp calculated it.
        table[index].score = static_cast<int16_t>(score);
        
        table[index].depth = static_cast<int8_t>(depth);
        table[index].flag = static_cast<uint8_t>(flag);
        table[index].age = current_age; 
    }

    inline bool probe(uint64_t hash, int depth, int alpha, int beta, int /*ply*/, int& return_score, MoveGen::Move& return_move, bool& has_move) {
        int index = hash & (TT_SIZE - 1);
        TTEntry entry = table[index];

        if (entry.hash == hash) {
            has_move = true;
            return_move = entry.best_move;

            if (entry.depth >= depth) {
                
                // Read the score exactly as stored. search.hpp handles the math now.
                int tt_score = entry.score;

                if (entry.flag == EXACT) {
                    return_score = tt_score;
                    return true;
                }
                
                if (entry.flag == UPPERBOUND && tt_score <= alpha) {
                    return_score = tt_score; 
                    return true;
                }
                if (entry.flag == LOWERBOUND && tt_score >= beta) {
                    return_score = tt_score; 
                    return true;
                }
            }
        }
        has_move = false;
        return false;
    }
}
