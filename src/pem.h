#ifndef PEM_H
#define PEM_H

#include "board.h"
#include <algorithm> // for std::min, std::max

const int PawnValue   = 100;
const int KnightValue = 320;
const int BishopValue = 330;
const int RookValue   = 500;
const int QueenValue  = 900;
const int KingValue   = 100000;

// Under the piece values so search.h can use it for move ordering!
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

// ==========================================
// --- MIDGAME PIECE SQUARE TABLES (PST) ---
// ==========================================

const int mg_pawnPST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0
};

// Midgame King: Hide behind the pawn shields!
const int mg_kingPST[64] = {
     20, 30, 10,  0,  0, 10, 30, 20, 
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20, 
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};

// ==========================================
// --- ENDGAME PIECE SQUARE TABLES (PST) ---
// ==========================================

// Endgame Pawns: Massively reward pushing pawns to promotion!
const int eg_pawnPST[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     10, 10, 10, 10, 10, 10, 10, 10,
     20, 20, 20, 20, 20, 20, 20, 20,
     30, 30, 30, 30, 30, 30, 30, 30,
     50, 50, 50, 50, 50, 50, 50, 50,
     80, 80, 80, 80, 80, 80, 80, 80,
    120,120,120,120,120,120,120,120,
      0,  0,  0,  0,  0,  0,  0,  0
};

// Endgame King: GET INTO THE CENTER AND FIGHT!
const int eg_kingPST[64] = {
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -50,-40,-30,-20,-20,-30,-40,-50
};

// ==========================================
// --- UNIVERSAL PIECE SQUARE TABLES ---
// ==========================================

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
      5, 10, 10, 10, 10, 10, 10,  5, 
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

// ==========================================
// --- PASSED PAWNS & KING SAFETY ---
// ==========================================

// Passed pawn bonuses based on Rank (0 to 7)
const int PASSED_PAWN_MG[8] = { 0, 5, 10, 20, 40, 70, 120, 0 };  // Middlegame
const int PASSED_PAWN_EG[8] = { 0, 10, 25, 50, 90, 150, 250, 0 }; // Endgame

inline bool isPassedPawn(const BoardState& board, int sq, int color) {
    int file = sq % 8;
    int rank = sq / 8;

    if (color == WHITE) {
        for (int r = rank + 1; r < 8; r++) {
            if (file > 0 && board.squares[r * 8 + (file - 1)] == B_PAWN) return false;
            if (board.squares[r * 8 + file] == B_PAWN) return false;
            if (file < 7 && board.squares[r * 8 + (file + 1)] == B_PAWN) return false;
        }
    } else {
        for (int r = rank - 1; r >= 0; r--) {
            if (file > 0 && board.squares[r * 8 + (file - 1)] == W_PAWN) return false;
            if (board.squares[r * 8 + file] == W_PAWN) return false;
            if (file < 7 && board.squares[r * 8 + (file + 1)] == W_PAWN) return false;
        }
    }
    return true; 
}

inline int evaluateKingSafety(const BoardState& board, int kingSq, int color) {
    int penalty = 0;
    int file = kingSq % 8;
    int rank = kingSq / 8;

    if (color == WHITE && rank > 1) return -50; // King exposed!
    if (color == BLACK && rank < 6) return -50; 

    int forwardDir = (color == WHITE) ? 1 : -1;
    int shieldRank = rank + forwardDir;

    if (shieldRank >= 0 && shieldRank <= 7) {
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            int piece = board.squares[shieldRank * 8 + f];
            int friendlyPawn = (color == WHITE) ? W_PAWN : B_PAWN;
            
            if (piece != friendlyPawn) {
                penalty -= 15; // Missing pawn shield
                
                bool fileOpen = true;
                for (int r = 1; r < 7; r++) {
                    if (board.squares[r * 8 + f] == W_PAWN || board.squares[r * 8 + f] == B_PAWN) {
                        fileOpen = false;
                        break;
                    }
                }
                if (fileOpen) penalty -= 30; // Open file targeting king!
            }
        }
    }
    return penalty;
}

// ==========================================
// --- MAIN EVALUATION FUNCTION ---
// ==========================================

inline int evaluate(const BoardState& board) {
    int mgScore = 0;
    int egScore = 0;
    int gamePhase = 0; 
    
    int whiteKingSq = -1;
    int blackKingSq = -1;

    for (int square = 0; square < 64; square++) {
        int piece = board.squares[square];
        if (piece == EMPTY) continue;

        int color = (piece >= W_PAWN && piece <= W_KING) ? WHITE : BLACK;
        int index = (color == WHITE) ? square : (square ^ 56);

        int mgValue = 0;
        int egValue = 0;

        switch (piece) {
            case W_PAWN:   case B_PAWN:   
                mgValue = PawnValue + mg_pawnPST[index];
                egValue = PawnValue + eg_pawnPST[index];
                if (color == WHITE && isPassedPawn(board, square, WHITE)) {
                    mgValue += PASSED_PAWN_MG[square / 8];
                    egValue += PASSED_PAWN_EG[square / 8];
                } else if (color == BLACK && isPassedPawn(board, square, BLACK)) {
                    mgValue += PASSED_PAWN_MG[7 - (square / 8)]; // Flip rank for black
                    egValue += PASSED_PAWN_EG[7 - (square / 8)];
                }
                break;
            case W_KNIGHT: case B_KNIGHT: 
                mgValue = KnightValue + knightPST[index];
                egValue = KnightValue + knightPST[index];
                gamePhase += 1; 
                break;
            case W_BISHOP: case B_BISHOP: 
                mgValue = BishopValue + bishopPST[index];
                egValue = BishopValue + bishopPST[index];
                gamePhase += 1; 
                break;
            case W_ROOK:   case B_ROOK:   
                mgValue = RookValue + rookPST[index];
                egValue = RookValue + rookPST[index];
                gamePhase += 2; 
                break;
            case W_QUEEN:  case B_QUEEN:  
                mgValue = QueenValue + queenPST[index];
                egValue = QueenValue + queenPST[index];
                gamePhase += 4; 
                break;
            case W_KING:   case B_KING:   
                mgValue = KingValue + mg_kingPST[index]; 
                egValue = KingValue + eg_kingPST[index]; 
                if (color == WHITE) whiteKingSq = square;
                else blackKingSq = square;
                break;
        }

        if (color == WHITE) {
            mgScore += mgValue;
            egScore += egValue;
        } else {
            mgScore -= mgValue;
            egScore -= egValue;
        }
    }

    // Apply King Safety penalties (ONLY to mgScore! It fades out automatically in the endgame)
    if (whiteKingSq != -1) mgScore += evaluateKingSafety(board, whiteKingSq, WHITE);
    if (blackKingSq != -1) mgScore -= evaluateKingSafety(board, blackKingSq, BLACK); // Subtracting a penalty adds points for White!

    // --- THE MAGIC BLEND (Tapered Eval) ---
    if (gamePhase > 24) gamePhase = 24;
    
    // Smoothly transition between Midgame and Endgame scores
    int finalScore = (mgScore * gamePhase + egScore * (24 - gamePhase)) / 24;

    // Return relative to the side whose turn it is
    return (board.sideToMove == WHITE) ? finalScore : -finalScore;
}

#endif
