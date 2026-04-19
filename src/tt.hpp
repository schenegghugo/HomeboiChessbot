#pragma once
#include "types.hpp"
#include "movegen.hpp"

const int TT_EXACT = 1;
const int TT_ALPHA = 2;
const int TT_BETA  = 3;

// 16 bytes per entry
struct TTEntry {
    U64 key;
    int score;
    uint16_t move; // compressed move
    uint8_t depth;
    uint8_t flag;
};

const int TT_SIZE = 1024 * 1024; // 1 Million Entries
inline TTEntry* TT = nullptr;

inline void init_tt() {
    if (!TT) TT = new TTEntry[TT_SIZE]();
}

inline void clear_tt() {
    for (int i = 0; i < TT_SIZE; ++i) {
        TT[i].key = 0;
        TT[i].flag = 0;
    }
}

// Compress move to fit inside TT nicely
inline uint16_t compress_move(const Move& m) {
    return (m.from) | (m.to << 6) | (m.promoted << 12);
}

// Save to Transposition Table
inline void tt_save(U64 hash, int depth, int score, int flag, Move best_move) {
    TTEntry& entry = TT[hash % TT_SIZE];
    entry.key = hash;
    entry.score = score;
    entry.depth = depth;
    entry.flag = flag;
    if (best_move.piece != EMPTY) {
        entry.move = compress_move(best_move);
    }
} // End namespace tt
