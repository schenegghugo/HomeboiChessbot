#ifndef PEM_H
#define PEM_H

#include "board.h"
#include <algorithm> 

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
// --- POSITIONAL BONUSES & PENALTIES ---
// ==========================================

const int BISHOP_PAIR_MG = 30;
const int BISHOP_PAIR_EG = 50;

const int ISOLATED_PAWN_MG = -15;
const int ISOLATED_PAWN_EG = -20;

const int DOUBLED_PAWN_MG = -15;
const int DOUBLED_PAWN_EG = -20;

const int ROOK_OPEN_FILE_MG = 20;
const int ROOK_OPEN_FILE_EG = 20;

const int ROOK_SEMI_OPEN_MG = 10;
const int ROOK_SEMI_OPEN_EG = 10;

// Passed pawn bonuses based on Rank (0 to 7)
const int PASSED_PAWN_MG[8] = { 0, 5, 10, 20, 40, 70, 120, 0 };  
const int PASSED_PAWN_EG[8] = { 0, 10, 25, 50, 90, 150, 250, 0 }; 

// ==========================================
// --- MIDGAME PIECE SQUARE TABLES ---
// ==========================================

const int mg_pawnPST[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

const int mg_knightPST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int mg_bishopPST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int mg_rookPST[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

const int mg_queenPST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int mg_kingPST[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

// ==========================================
// --- ENDGAME PIECE SQUARE TABLES ---
// ==========================================

const int eg_pawnPST[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
    120,120,120,120,120,120,120,120,
     80, 80, 80, 80, 80, 80, 80, 80,
     50, 50, 50, 50, 50, 50, 50, 50,
     30, 30, 30, 30, 30, 30, 30, 30,
     20, 20, 20, 20, 20, 20, 20, 20,
     10, 10, 10, 10, 10, 10, 10, 10,
      0,  0,  0,  0,  0,  0,  0,  0
};

const int eg_knightPST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int eg_bishopPST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int eg_rookPST[64] = {
     10, 10, 10, 10, 10, 10, 10, 10,
     20, 20, 20, 20, 20, 20, 20, 20,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
     -5, -5, -5, -5, -5, -5, -5, -5,
     -5, -5, -5, -5, -5, -5, -5, -5
};

const int eg_queenPST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  5, 10, 10, 10, 10,  5,-10,
     -5,  5, 10, 20, 20, 10,  5, -5,
      0,  5, 10, 20, 20, 10,  5, -5,
    -10,  5, 10, 10, 10, 10,  5,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

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
// --- HELPERS ---
// ==========================================

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

    if (color == WHITE && rank > 1) return -50; 
    if (color == BLACK && rank < 6) return -50; 

    int forwardDir = (color == WHITE) ? 1 : -1;
    int shieldRank = rank + forwardDir;

    if (shieldRank >= 0 && shieldRank <= 7) {
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            int piece = board.squares[shieldRank * 8 + f];
            int friendlyPawn = (color == WHITE) ? W_PAWN : B_PAWN;
            
            if (piece != friendlyPawn) {
                penalty -= 15; 
                
                bool fileOpen = true;
                for (int r = 1; r < 7; r++) {
                    if (board.squares[r * 8 + f] == W_PAWN || board.squares[r * 8 + f] == B_PAWN) {
                        fileOpen = false;
                        break;
                    }
                }
                if (fileOpen) penalty -= 30; 
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
    
    int wBishops = 0;
    int bBishops = 0;

    // Track pawn files for structure evaluation
    int wPawns[8] = {0};
    int bPawns[8] = {0};

    for (int sq = 0; sq < 64; sq++) {
        if (board.squares[sq] == W_PAWN) wPawns[sq % 8]++;
        else if (board.squares[sq] == B_PAWN) bPawns[sq % 8]++;
    }

    for (int square = 0; square < 64; square++) {
        int piece = board.squares[square];
        if (piece == EMPTY) continue;

        int color = (piece >= W_PAWN && piece <= W_KING) ? WHITE : BLACK;
        int index = (color == WHITE) ? square : (square ^ 56);
        int file = square % 8;
        int rank = square / 8;

        int mgValue = 0;
        int egValue = 0;

        switch (piece) {
            case W_PAWN:   
            case B_PAWN:   
                mgValue = PawnValue + mg_pawnPST[index];
                egValue = PawnValue + eg_pawnPST[index];
                
                // Passed Pawns
                if (isPassedPawn(board, square, color)) {
                    int r = (color == WHITE) ? rank : 7 - rank;
                    mgValue += PASSED_PAWN_MG[r];
                    egValue += PASSED_PAWN_EG[r];
                }

                // Doubled Pawns Penalty
                if ((color == WHITE && wPawns[file] > 1) || (color == BLACK && bPawns[file] > 1)) {
                    mgValue += DOUBLED_PAWN_MG;
                    egValue += DOUBLED_PAWN_EG;
                }

                // Isolated Pawns Penalty
                {
                    bool isolated = true;
                    if (color == WHITE) {
                        if (file > 0 && wPawns[file - 1] > 0) isolated = false;
                        if (file < 7 && wPawns[file + 1] > 0) isolated = false;
                    } else {
                        if (file > 0 && bPawns[file - 1] > 0) isolated = false;
                        if (file < 7 && bPawns[file + 1] > 0) isolated = false;
                    }
                    if (isolated) {
                        mgValue += ISOLATED_PAWN_MG;
                        egValue += ISOLATED_PAWN_EG;
                    }
                }
                break;

            case W_KNIGHT: 
            case B_KNIGHT: 
                mgValue = KnightValue + mg_knightPST[index];
                egValue = KnightValue + eg_knightPST[index];
                gamePhase += 1; 
                break;

            case W_BISHOP: 
            case B_BISHOP: 
                mgValue = BishopValue + mg_bishopPST[index];
                egValue = BishopValue + eg_bishopPST[index];
                gamePhase += 1; 
                if (color == WHITE) wBishops++;
                else bBishops++;
                break;

            case W_ROOK:   
            case B_ROOK:   
                mgValue = RookValue + mg_rookPST[index];
                egValue = RookValue + eg_rookPST[index];
                gamePhase += 2; 

                // Rooks on Open / Semi-Open Files
                if (wPawns[file] == 0 && bPawns[file] == 0) {
                    mgValue += ROOK_OPEN_FILE_MG;
                    egValue += ROOK_OPEN_FILE_EG;
                } else if ((color == WHITE && wPawns[file] == 0) || (color == BLACK && bPawns[file] == 0)) {
                    mgValue += ROOK_SEMI_OPEN_MG;
                    egValue += ROOK_SEMI_OPEN_EG;
                }
                break;

            case W_QUEEN:  
            case B_QUEEN:  
                mgValue = QueenValue + mg_queenPST[index];
                egValue = QueenValue + eg_queenPST[index];
                gamePhase += 4; 
                break;

            case W_KING:   
            case B_KING:   
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

    // --- POSITIONAL BONUSES FOR ENTIRE BOARD ---

    // The Bishop Pair
    if (wBishops >= 2) {
        mgScore += BISHOP_PAIR_MG;
        egScore += BISHOP_PAIR_EG;
    }
    if (bBishops >= 2) {
        mgScore -= BISHOP_PAIR_MG;
        egScore -= BISHOP_PAIR_EG;
    }

    // Apply King Safety penalties (ONLY to mgScore! It fades out naturally in the endgame)
    if (whiteKingSq != -1) mgScore += evaluateKingSafety(board, whiteKingSq, WHITE);
    if (blackKingSq != -1) mgScore -= evaluateKingSafety(board, blackKingSq, BLACK); 

    // --- THE MAGIC BLEND (Tapered Eval) ---
    if (gamePhase > 24) gamePhase = 24;
    
    // Smoothly transition between Midgame and Endgame scores
    int finalScore = (mgScore * gamePhase + egScore * (24 - gamePhase)) / 24;

    // Return relative to the side whose turn it is
    return (board.sideToMove == WHITE) ? finalScore : -finalScore;
}

#endif
