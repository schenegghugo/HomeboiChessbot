#ifndef UCI_H
#define UCI_H

#include "legalmoves.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include "board.h"
#include "movegen.h"
#include "makemove.h"
#include "search.h"

// Helper function: Convert a string like "e2e4" into our Move struct
inline Move parseMove(const std::string& moveStr, BoardState& board) {
    std::vector<Move> moves = getLegalMoves(board);
    for (Move m : moves) {
        if (moveToUCI(m) == moveStr) return m;
    }
    
    int fromCol = moveStr[0] - 'a';
    int fromRow = moveStr[1] - '1';
    int toCol   = moveStr[2] - 'a';
    int toRow   = moveStr[3] - '1';
    
    int fromSquare = fromRow * 8 + fromCol;
    int toSquare   = toRow * 8 + toCol;
    
    int flag = Flag_None;
    int promotedPiece = EMPTY;
    int movingPiece = board.squares[fromSquare];
    
    if ((movingPiece == W_KING || movingPiece == B_KING) && std::abs(fromCol - toCol) == 2) {
        flag = Flag_Castling;
    }
    
    if ((movingPiece == W_PAWN || movingPiece == B_PAWN) && fromCol != toCol && board.squares[toSquare] == EMPTY) {
        flag = Flag_EnPassant;
    }
    
    if (moveStr.length() == 5) {
        char promo = moveStr[4];
        int offset = (movingPiece == W_PAWN) ? 0 : 6;
        
        if (promo == 'q') promotedPiece = W_QUEEN + offset;
        else if (promo == 'r') promotedPiece = W_ROOK + offset;
        else if (promo == 'b') promotedPiece = W_BISHOP + offset;
        else if (promo == 'n') promotedPiece = W_KNIGHT + offset;
        
        flag = Flag_Promotion;
    }
    
    return Move(fromSquare, toSquare, flag, promotedPiece);
}

// The main UCI listening loop
inline void uciLoop() {
    BoardState board;
    std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    loadFEN(board, startFEN);

    std::string line;
    
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            std::cout << "id name Homeboi v1.2\n"; // Version 1.2 baby!
            std::cout << "id author Hugo Schenegg\n";
            std::cout << "option name Move Overhead type spin default 500 min 0 max 5000\n";
            std::cout << "option name Threads type spin default 1 min 1 max 128\n";
            std::cout << "option name Hash type spin default 16 min 1 max 1024\n";
            std::cout << "uciok\n";
        } 
        else if (command == "isready") {
            std::cout << "readyok\n";
        } 
        else if (command == "ucinewgame") {
            loadFEN(board, startFEN);
        } 
        else if (command == "position") {
            std::string token;
            iss >> token;
            
            if (token == "startpos") {
                loadFEN(board, startFEN);
                iss >> token; 
            } 
            else if (token == "fen") {
                std::string fen = "";
                for (int i = 0; i < 6; i++) { 
                    iss >> token;
                    fen += token + " ";
                }
                loadFEN(board, fen);
                iss >> token; 
            }
            
            while (iss >> token) {
                Move m = parseMove(token, board);
                makeMove(board, m);
            }
        }
        else if (command == "printmoves") {
            std::vector<Move> moves = getLegalMoves(board);
            for (Move m : moves) {
                std::cout << moveToUCI(m) << " ";
            }
            std::cout << "\nTotal moves: " << moves.size() << "\n";
        }
        // --- THE NEW TIME MANAGEMENT GO COMMAND ---
        else if (command == "go") {
            int wtime = 0, btime = 0, winc = 0, binc = 0;
            std::string token;
            
            // Read all the time data Lichess sends us
            while (iss >> token) {
                if (token == "wtime") iss >> wtime;
                else if (token == "btime") iss >> btime;
                else if (token == "winc") iss >> winc;
                else if (token == "binc") iss >> binc;
            }

            // Figure out whose time we are looking at
            int myTime = (board.sideToMove == WHITE) ? wtime : btime;
            int myInc  = (board.sideToMove == WHITE) ? winc : binc;

            long long timeLimitMs = 1000; // Default to 1 second if no time is provided

            if (myTime > 0) {
                // Formula: Use roughly 1/30th of our total time + half our increment
                timeLimitMs = (myTime / 30) + (myInc / 2);

                // Safety checks to absolutely prevent flagging
                if (timeLimitMs >= myTime) {
                    timeLimitMs = myTime - 500;
                }
                if (timeLimitMs < 50) {
                    timeLimitMs = 50; // Absolute minimum 50ms to think
                }
            }

            // Start Iterative Deepening Search!
            Move best = getBestMoveIterative(board, timeLimitMs);
            
            // Output the best move for Lichess
            if (best.fromSquare != 0 || best.toSquare != 0) {
                std::cout << "bestmove " << moveToUCI(best) << "\n";
            } else {
                std::cout << "bestmove 0000\n";
            }
        } 
        else if (command == "quit") {
            break;
        }
    }
}

#endif
