#ifndef SEARCH_H
#define SEARCH_H

#include "legalmoves.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include "board.h"
#include "movegen.h"
#include "makemove.h"
#include "pem.h"

// --- MOVE ORDERING (MVV-LVA) ---
// Calculate a "guess" score for a move. 
// We want to look at capturing a Queen with a Pawn FIRST!
inline int scoreMove(const BoardState& board, Move move) {
    int movingPiece = board.squares[move.fromSquare];
    int targetPiece = board.squares[move.toSquare];

    if (targetPiece != EMPTY) {
        int attackerValue = getPieceValue(movingPiece) / 100;
        int victimValue = getPieceValue(targetPiece) / 100;
        
        // Example: Pawn(1) takes Queen(9) = (10 * 9) - 1 = 89 (Searched immediately)
        // Example: Queen(9) takes Pawn(1) = (10 * 1) - 9 = 1 (Searched later)
        return (10 * victimValue) - attackerValue;
    }
    return 0; // Not a capture
}

// The Negamax Alpha-Beta Algorithm
inline int negamax(BoardState& board, int depth, int alpha, int beta) {
    // 1. Base Case: We hit our depth limit. Call the PEM!
    if (depth == 0) {
        return evaluate(board);
    }

    // Notice we use the fast PSEUDO-LEGAL generator inside the search tree!
    std::vector<Move> moves = generateMoves(board);
    
    // If there are no moves, it's either checkmate or stalemate
    if (moves.empty()) {
        return -99999 + (10 - depth); // Prefer longer survival
    }

    // --- THE MAGIC: SORT THE MOVES! ---
    std::sort(moves.begin(), moves.end(), [&board](Move a, Move b) {
        return scoreMove(board, a) > scoreMove(board, b);
    });

    int bestScore = -1000000; // Start with negative infinity

    // 2. Loop through all possible futures
    for (Move move : moves) {
        
        // --- KING CAPTURE DETECTION (THE SPEED TRICK) ---
        // Because our move generator is "pseudo-legal", it generates moves
        // even if it leaves our King in check. 
        // If we see we can eat the opponent's King in this timeline, 
        // this timeline is an INSTANT WIN. No need to look further!
        int targetPiece = board.squares[move.toSquare];
        if (targetPiece == W_KING || targetPiece == B_KING) {
            return 100000; 
        }

        // Time Travel: Jump into the future
        UndoData undo = makeMove(board, move);
        
        // Recursion
        int currentScore = -negamax(board, depth - 1, -beta, -alpha);
        
        // Time Travel: Rewind the board
        undoMove(board, move, undo);

        // 3. Alpha-Beta Pruning
        if (currentScore > bestScore) {
            bestScore = currentScore;
        }
        if (bestScore > alpha) {
            alpha = bestScore;
        }
        if (alpha >= beta) {
            break; // The opponent would never let us get this timeline. Prune it!
        }
    }

    return bestScore;
}

// The function we call from main.cpp to find the best move
inline Move getBestMove(BoardState& board, int depth) {
    // 1. USE STRICT LEGAL MOVES ONLY AT THE ROOT!
    // This physically prevents the engine from choosing a move that leaves us in check.
    std::vector<Move> moves = getLegalMoves(board); 
    
    // 2. CHECKMATE PREVENTION: If there are absolutely no legal moves,
    // the game is over. Return a dummy move so we don't crash trying to access moves[0].
    if (moves.empty()) {
        std::cout << "bestmove 0000\n";
        return Move(0, 0, Flag_None, EMPTY); 
    }

    // --- SORT ROOT MOVES TOO ---
    std::sort(moves.begin(), moves.end(), [&board](Move a, Move b) {
        return scoreMove(board, a) > scoreMove(board, b);
    });

    Move bestMove = moves[0];
    int bestScore = -1000000;

    for (Move move : moves) {
        UndoData undo = makeMove(board, move);
        
        // Evaluate this move using Alpha-Beta
        int score = -negamax(board, depth - 1, -1000000, 1000000);
        
        undoMove(board, move, undo);

        // Update the best move if we found a better score
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    std::cout << "info depth " << depth << " score cp " << bestScore << "\n";    
    return bestMove;
}

#endif
