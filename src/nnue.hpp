#pragma once
#include "board.hpp"
#include <algorithm>
#include <immintrin.h> // AVX2

constexpr int L1_SIZE = 256;

// Memory Aligned Weights (Must perfectly match export_net.cpp!)
struct alignas(64) NNUEWeights {
    alignas(32) int16_t ft_weights[64][768][L1_SIZE];
    alignas(32) int16_t ft_biases[L1_SIZE];
    
    alignas(32) int16_t output_weights[L1_SIZE * 2]; // 512
    int32_t output_bias;
};

// Global Weights (Loaded from chessboi.bin)
extern NNUEWeights weights;

// The Accumulator Stack
struct Accumulator {
    alignas(32) int16_t white[L1_SIZE];
    alignas(32) int16_t black[L1_SIZE];
};
extern Accumulator accumulators[MAX_PLY];

// --- NNUE CORE FUNCTIONS ---

// O(N) Refresh using Lightning-Fast Bitboards
inline void refresh_accumulator(const Board& board, int ply) {
    for (int i = 0; i < L1_SIZE; i += 16) {
        __m256i bias = _mm256_load_si256((__m256i*)&weights.ft_biases[i]);
        _mm256_store_si256((__m256i*)&accumulators[ply].white[i], bias);
        _mm256_store_si256((__m256i*)&accumulators[ply].black[i], bias);
    }

    // PERSPECTIVE FLIP: Black's king square is seen from Black's side
    int w_k = board.white_king_sq;
    int b_k = flip_sq(board.black_king_sq);

    for (int piece = 0; piece < 12; ++piece) {
        U64 bb = board.pieces[piece];
        while (bb) {
            int sq = pop_lsb(bb);
            
            // INDICES SPLIT FOR PERSPECTIVE
            int w_idx = piece * 64 + sq;
            int b_idx = flip_piece(piece) * 64 + flip_sq(sq);

            for (int i = 0; i < L1_SIZE; i += 16) {
                __m256i w_acc = _mm256_load_si256((__m256i*)&accumulators[ply].white[i]);
                w_acc = _mm256_add_epi16(w_acc, _mm256_load_si256((__m256i*)&weights.ft_weights[w_k][w_idx][i]));
                _mm256_store_si256((__m256i*)&accumulators[ply].white[i], w_acc);

                __m256i b_acc = _mm256_load_si256((__m256i*)&accumulators[ply].black[i]);
                b_acc = _mm256_add_epi16(b_acc, _mm256_load_si256((__m256i*)&weights.ft_weights[b_k][b_idx][i]));
                _mm256_store_si256((__m256i*)&accumulators[ply].black[i], b_acc);
            }
        }
    }
}

// O(1) Update 
inline void update_accumulator(int ply, const Move& m, const Board& board) {
    // EN PASSANT SAFETY NET: 
    // If a pawn captures, but the captured piece is NOT located on the destination square, it's En Passant!
    bool is_en_passant = (type_of(m.piece) == PAWN && m.captured != EMPTY && ((board.pieces[m.captured] & (1ULL << m.to)) == 0));

    // Full refresh if King moves (changing King bucket) or if En Passant happened
    if (m.piece == wK || m.piece == bK || is_en_passant) {
        refresh_accumulator(board, ply);
        return;
    }

    // Copy accumulator state from the previous ply
    accumulators[ply] = accumulators[ply - 1];

    auto update_piece = [&](int piece, int sq, bool is_adding) {
        // INDICES AND KINGS SPLIT FOR PERSPECTIVE
        int w_idx = piece * 64 + sq;
        int b_idx = flip_piece(piece) * 64 + flip_sq(sq);

        int w_k = board.white_king_sq;
        int b_k = flip_sq(board.black_king_sq);

        for (int i = 0; i < L1_SIZE; i += 16) {
            __m256i w_acc = _mm256_load_si256((__m256i*)&accumulators[ply].white[i]);
            __m256i w_wt  = _mm256_load_si256((__m256i*)&weights.ft_weights[w_k][w_idx][i]);
            w_acc = is_adding ? _mm256_add_epi16(w_acc, w_wt) : _mm256_sub_epi16(w_acc, w_wt);
            _mm256_store_si256((__m256i*)&accumulators[ply].white[i], w_acc);

            __m256i b_acc = _mm256_load_si256((__m256i*)&accumulators[ply].black[i]);
            __m256i b_wt  = _mm256_load_si256((__m256i*)&weights.ft_weights[b_k][b_idx][i]);
            b_acc = is_adding ? _mm256_add_epi16(b_acc, b_wt) : _mm256_sub_epi16(b_acc, b_wt);
            _mm256_store_si256((__m256i*)&accumulators[ply].black[i], b_acc);
        }
    };

    // Remove the piece from its original square
    update_piece(m.piece, m.from, false); 
    
    // Add the piece to its destination square (handle promotion if necessary)
    update_piece((m.promoted != EMPTY) ? m.promoted : m.piece, m.to, true); 

    // Remove the captured piece if there was one
    if (m.captured != EMPTY) {
        update_piece(m.captured, m.to, false); 
    }
}

// 2-Layer Forward Pass (Leaf Node)
inline int evaluate_nnue(int ply, int side_to_move) {
    int16_t* us = (side_to_move == WHITE) ? accumulators[ply].white : accumulators[ply].black;
    int16_t* them = (side_to_move == WHITE) ? accumulators[ply].black : accumulators[ply].white;

    int64_t out = weights.output_bias;

    // Evaluate both sides using AVX-compatible loops and the 255 ceiling (Clipped ReLU)
    for (int i = 0; i < L1_SIZE; ++i) {
        int64_t us_act = std::clamp<int16_t>(us[i], 0, 255);
        out += us_act * weights.output_weights[i];

        int64_t them_act = std::clamp<int16_t>(them[i], 0, 255);
        out += them_act * weights.output_weights[i + L1_SIZE];
    }

    // Convert raw NN output back down into Centipawns
    return out / 16320;
}
