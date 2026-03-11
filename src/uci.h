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
    // Use strictly legal moves here so the GUI's moves map perfectly to ours
    std::vector<Move> moves = getLegalMoves(board);
    for (Move m : moves) {
        if (moveToUCI(m) == moveStr) return m;
    }
    
    // --- PARALLEL UNIVERSE PREVENTION ---
    // If the GUI played a move we haven't written generation logic for yet (like Castling),
    // we CANNOT return moves[0] because it corrupts the entire timeline.
    // Instead, we forcefully calculate the exact move the GUI sent!
    
    int fromCol = moveStr[0] - 'a';
    int fromRow = moveStr[1] - '1';
    int toCol   = moveStr[2] - 'a';
    int toRow   = moveStr[3] - '1';
    
    int fromSquare = fromRow * 8 + fromCol;
    int toSquare   = toRow * 8 + toCol;
    
    int flag = Flag_None;
    int promotedPiece = EMPTY;
    int movingPiece = board.squares[fromSquare];
    
    // 1. Detect if the GUI castled (King jumped 2 squares left or right)
    if ((movingPiece == W_KING || movingPiece == B_KING) && std::abs(fromCol - toCol) == 2) {
        flag = Flag_Castling;
    }
    
    // 2. Detect if the GUI En Passanted (Pawn moved diagonally to an empty square)
    if ((movingPiece == W_PAWN || movingPiece == B_PAWN) && fromCol != toCol && board.squares[toSquare] == EMPTY) {
        flag = Flag_EnPassant;
    }
    
    // 3. Detect if the GUI Promoted a piece
    if (moveStr.length() == 5) {
        char promo = moveStr[4];
        int offset = (movingPiece == W_PAWN) ? 0 : 6; // White pieces vs Black pieces
        
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
    // Standard starting position FEN
    std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    loadFEN(board, startFEN);

    std::string line;
    
    // Listen for commands forever until "quit"
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            std::cout << "id name TalBot\n";
            std::cout << "id author Hugo\n";
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
                iss >> token; // Consume the word "moves" if it exists
            } 
            else if (token == "fen") {
                std::string fen = "";
                // FEN has 6 parts, grab them all
                for (int i = 0; i < 6; i++) { 
                    iss >> token;
                    fen += token + " ";
                }
                loadFEN(board, fen);
                iss >> token; // Consume the word "moves"
            }
            
            // Apply all moves the GUI sends us to update our board
            while (iss >> token) {
                Move m = parseMove(token, board);
                makeMove(board, m);
            }
        }

        else if (command == "printmoves") {
            // Updated to print STRICTLY legal moves to help with debugging
            std::vector<Move> moves = getLegalMoves(board);
            for (Move m : moves) {
                std::cout << moveToUCI(m) << " ";
            }
            std::cout << "\nTotal moves: " << moves.size() << "\n";
        }

        else if (command == "go") {
            // For now, we will force the engine to always search Depth 5
            Move best = getBestMove(board, 5);
            
            // If best move is 0000, it means we are mated/stalemated. 
            // We only print bestmove if it's a real move.
            if (best.fromSquare != 0 || best.toSquare != 0) {
                std::cout << "bestmove " << moveToUCI(best) << "\n";
            }
        } 
        else if (command == "quit") {
            break;
        }
    }
}

#endif
