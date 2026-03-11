#ifndef PEM_H
#define PEM_H

#include "board.h"
#include <cmath>

const int PawnValue   = 100;
const int KnightValue = 320;
const int BishopValue = 330;
const int RookValue   = 500;
const int QueenValue  = 900;
const int KingValue   = 100000;

// --- PIECE SQUARE TABLES (PST) ---
// These act as "Heat Maps" for the pieces.
// Values are from White's perspective. (Index 0 = a1, Index 63 = h8)

const int pawnPST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0, // Rank 1
     5, 10, 10,-20,-20, 10, 10,  5, // Rank 2
     5, -5,-10,  0,  0,-10, -5,  5, // Rank 3
     0,  0,  0, 20, 20,  0,  0,  0, // Rank 4
     5,  5, 10, 25, 25, 10,  5,  5, // Rank 5
    10, 10, 20, 30, 30, 20, 10, 10, // Rank 6
    50, 50, 50, 50, 50, 50, 50, 50, // Rank 7
     0,  0,  0,  0,  0,  0,  0,  0  // Rank 8
};

const int knightPST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int bishopPST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int rookPST[64] = {
      0,  0,  5, 10, 10,  5,  0,  0,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      5, 10, 10, 10, 10, 10, 10,  5, // Rooks love the 7th rank!
      0,  0,  0,  0,  0,  0,  0,  0
};

const int queenPST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int kingPST[64] = {
     20, 30, 10,  0,  0, 10, 30, 20, // Loves Castling (g1/c1)!
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20, // Terrified of the center
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};

inline int getPieceValue(int piece) {
    switch (piece) {
        case W_PAWN:   case B_PAWN:   return PawnValue;
        case W_KNIGHT: case B_KNIGHT: return KnightValue;
        case W_BISHOP: case B_BISHOP: return BishopValue;
        case W_ROOK:   case B_ROOK:   return RookValue;
        case W_QUEEN:  case B_QUEEN:  return QueenValue;
        case W_KING:   case B_KING:   return KingValue;
        default: return 0;
    }
}

// Helper function to read the heat map
inline int getPSTValue(int piece, int square) {
    int color = (piece >= W_PAWN && piece <= W_KING) ? WHITE : BLACK;
    
    // The magic trick: If the piece is Black, we flip the board upside down!
    // square ^ 56 perfectly mirrors a1 to a8, b1 to b8, etc.
    int index = (color == WHITE) ? square : (square ^ 56);
    
    switch (piece) {
        case W_PAWN:   case B_PAWN:   return pawnPST[index];
        case W_KNIGHT: case B_KNIGHT: return knightPST[index];
        case W_BISHOP: case B_BISHOP: return bishopPST[index];
        case W_ROOK:   case B_ROOK:   return rookPST[index];
        case W_QUEEN:  case B_QUEEN:  return queenPST[index];
        case W_KING:   case B_KING:   return kingPST[index];
        default: return 0;
    }
}

inline int evaluate(const BoardState& board) {
    int score = 0;

    for (int square = 0; square < 64; square++) {
        int piece = board.squares[square];
        if (piece == EMPTY) continue;

        int color = (piece >= W_PAWN && piece <= W_KING) ? WHITE : BLACK;
        
        // Value = Material + Positional Heat Map
        int value = getPieceValue(piece) + getPSTValue(piece, square);

        if (color == WHITE) {
            score += value;
        } else {
            score -= value;
        }
    }

    // Return relative to the side whose turn it is
    return (board.sideToMove == WHITE) ? score : -score;
}

#endif
