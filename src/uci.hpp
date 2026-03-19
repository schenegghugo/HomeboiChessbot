#pragma once

#include "board.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "book.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>

namespace UCI {

    inline void load_fen(Board& board, const std::string& fen) {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 6; ++j) board.pieces[i][j] = 0ULL;
            board.occupancies[i] = 0ULL;
        }
        board.occupancies[static_cast<int>(Color::Both)] = 0ULL;

        int file = 0, rank = 7;
        size_t i = 0;

        for (; i < fen.length() && fen[i] != ' '; i++) {
            char c = fen[i];
            if (c == '/') {
                rank--;
                file = 0;
            } else if (isdigit(c)) {
                file += (c - '0');
            } else {
                Color col = isupper(c) ? Color::White : Color::Black;
                int pt = PAWN;
                char lc = tolower(c);
                if (lc == 'n') pt = KNIGHT;
                else if (lc == 'b') pt = BISHOP;
                else if (lc == 'r') pt = ROOK;
                else if (lc == 'q') pt = QUEEN;
                else if (lc == 'k') pt = KING;

                int sq = rank * 8 + file;
                board.pieces[static_cast<int>(col)][pt] |= (1ULL << sq);
                file++;
            }
        }

        for (int c = 0; c < 2; ++c) {
            for (int pt = 0; pt < 6; ++pt) {
                board.occupancies[c] |= board.pieces[c][pt];
            }
        }
        board.occupancies[static_cast<int>(Color::Both)] =
            board.occupancies[static_cast<int>(Color::White)] |
            board.occupancies[static_cast<int>(Color::Black)];

        if (i >= fen.length()) return;
        i++; 

        if (i < fen.length()) {
            board.side_to_move = (fen[i] == 'w') ? Color::White : Color::Black;
            i++; 
        }

        if (i >= fen.length()) return;
        i++; 

        board.castling_rights = 0;
        if (i < fen.length() && fen[i] == '-') {
            i++;
        } else {
            while (i < fen.length() && fen[i] != ' ') {
                if (fen[i] == 'K') board.castling_rights |= 1;
                else if (fen[i] == 'Q') board.castling_rights |= 2;
                else if (fen[i] == 'k') board.castling_rights |= 4;
                else if (fen[i] == 'q') board.castling_rights |= 8;
                i++;
            }
        }

        if (i >= fen.length()) return;
        i++; 

        board.en_passant_square = -1;
        if (i < fen.length() && fen[i] != '-') {
            if (i + 1 < fen.length()) {
                int f = fen[i] - 'a';
                int r = fen[i+1] - '1';
                if (f >= 0 && f <= 7 && r >= 0 && r <= 7) {
                    board.en_passant_square = r * 8 + f;
                }
            }
        }
    }

    inline MoveGen::Move parse_move(const std::string& move_str, Board& board) {
        MoveGen::MoveList moves = MoveGen::generate_pseudo_legal_moves(board);

        for (int i = 0; i < moves.count; ++i) {
            MoveGen::Move move = moves.moves[i];
            
            // Using helper method
            if (MoveGen::move_to_string(move) == move_str) {
                Board copy = board;
                if (MoveGen::make_move(copy, move)) return move;
            }
        }

        if (move_str.length() >= 4) {
            int src_file = move_str[0] - 'a';
            int src_rank = move_str[1] - '1';
            int tgt_file = move_str[2] - 'a';
            int tgt_rank = move_str[3] - '1';

            if (src_file >= 0 && src_file <= 7 && src_rank >= 0 && src_rank <= 7 &&
                tgt_file >= 0 && tgt_file <= 7 && tgt_rank >= 0 && tgt_rank <= 7) {

                int src_sq = src_rank * 8 + src_file;
                int tgt_sq = tgt_rank * 8 + tgt_file;

                for (int i = 0; i < moves.count; ++i) {
                    MoveGen::Move move = moves.moves[i];
                    
                    // Using helper methods
                    if (MoveGen::get_source(move) == src_sq && MoveGen::get_target(move) == tgt_sq) {
                        Board copy = board;
                        if (MoveGen::make_move(copy, move)) return move;
                    }
                }
            }
        }

        return 0; // Return 0 (empty move) if not found
    }

    inline void uci_loop() {
        Board board;
        std::string start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

        load_fen(board, start_fen);

        std::string line;

        while (std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "uci") {
                std::cout << "id name Chessboi v2.0\n";
                std::cout << "id author Hugo\n";
                std::cout << "uciok\n";
            }
            else if (command == "isready") {
                std::cout << "readyok\n";
            }
            else if (command == "ucinewgame") {
                TT::clear_tt(); 
                load_fen(board, start_fen);
            }
            else if (command == "position") {
                std::string token;
                iss >> token;

                if (token == "startpos") {
                    load_fen(board, start_fen);
                    iss >> token; 
                }
                else if (token == "fen") {
                    std::string fen = "";
                    while (iss >> token && token != "moves") {
                        fen += token + " ";
                    }
                    load_fen(board, fen);
                }

                if (token == "moves") {
                    while (iss >> token) {
                        MoveGen::Move m = parse_move(token, board);

                        // If m != 0, it's a valid move
                        if (m != 0) {
                            MoveGen::make_move(board, m);
                        } else {
                            std::cout << "info string ERROR: Chessboi failed to parse " << token << std::endl;
                            break;
                        }
                    }
                }
            }
            else if (command == "go") {
                int wtime = 0, btime = 0, winc = 0, binc = 0;
                long long movetime = 0;
                std::string token;

                while (iss >> token) {
                    if (token == "wtime") iss >> wtime;
                    else if (token == "btime") iss >> btime;
                    else if (token == "winc") iss >> winc;
                    else if (token == "binc") iss >> binc;
                    else if (token == "movetime") iss >> movetime;
                }

                long long time_limit_ms = 1000;

                if (movetime > 0) {
                    time_limit_ms = movetime - 50;
                    if (time_limit_ms < 1) time_limit_ms = 1;
                } else {
                    int my_time = (board.side_to_move == Color::White) ? wtime : btime;
                    int my_inc  = (board.side_to_move == Color::White) ? winc : binc;

                    if (my_time > 0) {
                        time_limit_ms = (my_time / 30) + (my_inc / 2);

                        if (my_time < 10000) {
                            time_limit_ms = 100;
                        }
                        if (my_time < 500) {
                            time_limit_ms = 20; 
                        }
                        if (time_limit_ms > my_time - 250) {
                            time_limit_ms = my_time - 250;
                        }
                        if (time_limit_ms < 10) time_limit_ms = 10;
                    }
                }

                MoveGen::Move book_move = 0;
                if (Book::get_book_move(board, book_move)) {
                    std::cout << "info string Playing book move" << std::endl;
                    std::cout << "bestmove " << MoveGen::move_to_string(book_move) << std::endl;
                } else {
                    MoveGen::Move best = Search::search_best_move(board, time_limit_ms);

                    // Checking validity of returned search move is clean!
                    if (best != 0) {
                        std::cout << "bestmove " << MoveGen::move_to_string(best) << std::endl;
                    } else {
                        std::cout << "bestmove 0000" << std::endl; 
                    }
                }
            }
            else if (command == "quit") {
                break;
            }
        }
    }

} // End namespace
