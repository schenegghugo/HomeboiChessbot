#pragma once

#include "board.hpp"
#include "movegen.hpp"
#include "polyglot_keys.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>

inline uint64_t swapEndian64(uint64_t val) { return __builtin_bswap64(val); }
inline uint16_t swapEndian16(uint16_t val) { return __builtin_bswap16(val); }

#pragma pack(push, 1)
struct PolyglotEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};
#pragma pack(pop)

namespace Book {

    inline std::vector<PolyglotEntry>& get_memory() {
        static std::vector<PolyglotEntry> book;
        return book;
    }

    inline void load(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cout << "info string Warning: Opening book not found at " << path << std::endl;
            return;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        size_t num_entries = size / sizeof(PolyglotEntry);
        auto& book = get_memory();
        book.resize(num_entries);
        
        file.read(reinterpret_cast<char*>(book.data()), size);
        file.close();

        for (auto& entry : book) {
            entry.key = swapEndian64(entry.key);
            entry.move = swapEndian16(entry.move);
            entry.weight = swapEndian16(entry.weight);
        }

        std::cout << "info string Loaded " << num_entries << " book entries into RAM." << std::endl;
    }

    inline uint64_t compute_hash(const Board& board) {
        uint64_t hash = 0;

        for (int color = 0; color < 2; ++color) {
            for (int pt = 0; pt < 6; ++pt) {
                uint64_t bb = board.pieces[color][pt];
                int poly_piece = (pt * 2) + ((color == static_cast<int>(Color::White)) ? 1 : 0);
                
                while (bb) {
                    int sq = __builtin_ctzll(bb);
                    hash ^= PolyglotRandom64[64 * poly_piece + sq];
                    bb &= bb - 1;
                }
            }
        }

        if (board.castling_rights & 1) hash ^= PolyglotRandom64[768]; 
        if (board.castling_rights & 2) hash ^= PolyglotRandom64[769]; 
        if (board.castling_rights & 4) hash ^= PolyglotRandom64[770]; 
        if (board.castling_rights & 8) hash ^= PolyglotRandom64[771]; 

        if (board.en_passant_square != -1) {
            int ep_file = board.en_passant_square % 8;
            int us = static_cast<int>(board.side_to_move);
            uint64_t our_pawns = board.pieces[us][PAWN];
            
            int pawn_rank = (board.side_to_move == Color::White) ? 4 : 3;
            
            bool adjacent_enemy = false;
            if (ep_file > 0 && (our_pawns & (1ULL << (pawn_rank * 8 + ep_file - 1)))) adjacent_enemy = true;
            if (ep_file < 7 && (our_pawns & (1ULL << (pawn_rank * 8 + ep_file + 1)))) adjacent_enemy = true;

            if (adjacent_enemy) {
                hash ^= PolyglotRandom64[772 + ep_file];
            }
        }

        if (board.side_to_move == Color::White) {
            hash ^= PolyglotRandom64[780];
        }

        return hash;
    }

    inline bool get_book_move(const Board& board, MoveGen::Move& out_move) {
        const auto& book = get_memory();
        if (book.empty()) return false;

        uint64_t poly_hash = compute_hash(board);

        auto it = std::lower_bound(book.begin(), book.end(), poly_hash,
            [](const PolyglotEntry& entry, uint64_t hash) {
                return entry.key < hash;
            });

        if (it == book.end() || it->key != poly_hash) {
            return false;
        }

        std::vector<PolyglotEntry> valid_moves;
        int total_weight = 0;
        
        while (it != book.end() && it->key == poly_hash) {
            valid_moves.push_back(*it);
            total_weight += it->weight;
            ++it;
        }

        if (valid_moves.empty() || total_weight == 0) return false;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, total_weight - 1);
        int random_choice = dist(gen);
        
        int running_weight = 0;
        uint16_t chosen_poly_move = valid_moves[0].move;
        for (const auto& entry : valid_moves) {
            running_weight += entry.weight;
            if (random_choice < running_weight) {
                chosen_poly_move = entry.move;
                break;
            }
        }

        int poly_to = chosen_poly_move & 63;
        int poly_from = (chosen_poly_move >> 6) & 63;
        int poly_promo = (chosen_poly_move >> 12) & 7; // 1=N, 2=B, 3=R, 4=Q

        // 0 is our new default "no promotion" state
        int engine_promo = 0; 
        if (poly_promo >= 1 && poly_promo <= 4) engine_promo = poly_promo;

        int us = static_cast<int>(board.side_to_move);
        if (board.pieces[us][KING] & (1ULL << poly_from)) {
            if (poly_from == 4 && poly_to == 7) poly_to = 6;   
            if (poly_from == 4 && poly_to == 0) poly_to = 2;   
            if (poly_from == 60 && poly_to == 63) poly_to = 62; 
            if (poly_from == 60 && poly_to == 56) poly_to = 58; 
        }

        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);
        for (int i = 0; i < moves.count; ++i) {
            MoveGen::Move m = moves.moves[i];
            
            // Using bitwise getters
            if (MoveGen::get_source(m) == poly_from && MoveGen::get_target(m) == poly_to) {
                if (engine_promo == 0 || MoveGen::get_promoted_piece(m) == engine_promo) {
                    out_move = m;
                    return true;
                }
            }
        }

        return false;
    }
} // namespace Book
