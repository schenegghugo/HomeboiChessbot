#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <vector>
#include <cmath>
#include "board.h"
#include "move.h"
#include "precomputed.h"

inline bool isColor(int piece, int color) {
    if (piece == EMPTY) return false;
    if (color == WHITE) return (piece >= W_PAWN && piece <= W_KING);
    else return (piece >= B_PAWN && piece <= B_KING);
}

inline bool isEnemy(int piece, int friendlyColor) {
    if (piece == EMPTY) return false;
    int enemyColor = (friendlyColor == WHITE) ? BLACK : WHITE;
    return isColor(piece, enemyColor);
}

// NEW: Highly optimized function to check if a square is attacked by the enemy.
// This is MANDATORY for Castling (King cannot pass through or into check).
inline bool isSquareAttacked(const BoardState& board, int square, int attackerColor) {
    // 1. Pawns (Look backwards to see if an enemy pawn is attacking this square)
    int pawnOffsets[2] = { (attackerColor == WHITE) ? -7 : 7, (attackerColor == WHITE) ? -9 : 9 };
    int attackerPawn = (attackerColor == WHITE) ? W_PAWN : B_PAWN;
    for (int i = 0; i < 2; i++) {
        int target = square + pawnOffsets[i];
        // Ensure it doesn't wrap around the board edges
        if (target >= 0 && target < 64 && std::abs((square % 8) - (target % 8)) == 1) {
            if (board.squares[target] == attackerPawn) return true;
        }
    }

    // 2. Knights
    int attackerKnight = (attackerColor == WHITE) ? W_KNIGHT : B_KNIGHT;
    for (int i = 0; i < NumKnightMoves[square]; i++) {
        if (board.squares[KnightMoves[square][i]] == attackerKnight) return true;
    }

    // 3. Kings
    int attackerKing = (attackerColor == WHITE) ? W_KING : B_KING;
    for (int dir = 0; dir < 8; dir++) {
        if (NumSquaresToEdge[square][dir] > 0) {
            if (board.squares[square + DirectionOffsets[dir]] == attackerKing) return true;
        }
    }

    // 4. Sliding Pieces (Bishops, Rooks, Queens)
    for (int dir = 0; dir < 8; dir++) {
        bool isDiagonal = (dir >= 4); // 0-3 are Orthogonal (Rooks), 4-7 are Diagonal (Bishops)
        int limit = NumSquaresToEdge[square][dir];
        int current = square;
        for (int i = 0; i < limit; i++) {
            current += DirectionOffsets[dir];
            int piece = board.squares[current];
            if (piece != EMPTY) {
                if (isColor(piece, attackerColor)) {
                    if (isDiagonal && (piece == W_BISHOP || piece == B_BISHOP || piece == W_QUEEN || piece == B_QUEEN)) return true;
                    if (!isDiagonal && (piece == W_ROOK || piece == B_ROOK || piece == W_QUEEN || piece == B_QUEEN)) return true;
                }
                break; // Blocked by a piece (friend or foe), stop sliding
            }
        }
    }
    return false;
}

inline std::vector<Move> generateMoves(const BoardState& board) {
    std::vector<Move> moves;
    moves.reserve(256); 
    int friendlyColor = board.sideToMove;

    for (int square = 0; square < 64; square++) {
        int piece = board.squares[square];

        if (piece == EMPTY || !isColor(piece, friendlyColor)) continue;

        // ==========================================
        // PART 1: SLIDING PIECES
        // ==========================================
        bool isSlidingPiece = (piece == W_BISHOP || piece == B_BISHOP || 
                               piece == W_ROOK   || piece == B_ROOK || 
                               piece == W_QUEEN  || piece == B_QUEEN);

        if (isSlidingPiece) {
            int startDirIndex = (piece == W_BISHOP || piece == B_BISHOP) ? 4 : 0;
            int endDirIndex   = (piece == W_ROOK   || piece == B_ROOK)   ? 4 : 8;

            for (int dir = startDirIndex; dir < endDirIndex; dir++) {
                int limit = NumSquaresToEdge[square][dir];
                int targetSquare = square; 
                int offset = DirectionOffsets[dir];

                for (int i = 0; i < limit; i++) {
                    targetSquare += offset; 
                    int pieceAtTarget = board.squares[targetSquare];

                    if (pieceAtTarget == EMPTY) {
                        moves.push_back(Move(square, targetSquare));
                    } 
                    else {
                        if (isEnemy(pieceAtTarget, friendlyColor)) {
                            moves.push_back(Move(square, targetSquare));
                        }
                        break; 
                    }
                }
            }
        }

        // ==========================================
        // PART 2: KING MOVES (AND CASTLING!)
        // ==========================================
        bool isKing = (piece == W_KING || piece == B_KING);
        if (isKing) {
            for (int dir = 0; dir < 8; dir++) {
                if (NumSquaresToEdge[square][dir] == 0) continue;
                int targetSquare = square + DirectionOffsets[dir];
                int pieceAtTarget = board.squares[targetSquare];

                if (pieceAtTarget == EMPTY || isEnemy(pieceAtTarget, friendlyColor)) {
                    moves.push_back(Move(square, targetSquare));
                }
            }
            
            // --- NEW: CASTLING LOGIC ---
            int enemyColor = (friendlyColor == WHITE) ? BLACK : WHITE;
            
            if (friendlyColor == WHITE) {
                // Kingside (e1 to g1)
                if (board.whiteCastleK) {
                    if (board.squares[5] == EMPTY && board.squares[6] == EMPTY) { // f1, g1 empty
                        if (!isSquareAttacked(board, 4, enemyColor) &&  // e1 not attacked
                            !isSquareAttacked(board, 5, enemyColor) &&  // f1 not attacked
                            !isSquareAttacked(board, 6, enemyColor)) {  // g1 not attacked
                            moves.push_back(Move(4, 6, Flag_Castling));
                        }
                    }
                }
                // Queenside (e1 to c1)
                if (board.whiteCastleQ) {
                    if (board.squares[3] == EMPTY && board.squares[2] == EMPTY && board.squares[1] == EMPTY) { // d1, c1, b1 empty
                        if (!isSquareAttacked(board, 4, enemyColor) &&  // e1 not attacked
                            !isSquareAttacked(board, 3, enemyColor) &&  // d1 not attacked
                            !isSquareAttacked(board, 2, enemyColor)) {  // c1 not attacked
                            moves.push_back(Move(4, 2, Flag_Castling));
                        }
                    }
                }
            } else {
                // Kingside (e8 to g8)
                if (board.blackCastleK) {
                    if (board.squares[61] == EMPTY && board.squares[62] == EMPTY) {
                        if (!isSquareAttacked(board, 60, enemyColor) && 
                            !isSquareAttacked(board, 61, enemyColor) && 
                            !isSquareAttacked(board, 62, enemyColor)) {
                            moves.push_back(Move(60, 62, Flag_Castling));
                        }
                    }
                }
                // Queenside (e8 to c8)
                if (board.blackCastleQ) {
                    if (board.squares[59] == EMPTY && board.squares[58] == EMPTY && board.squares[57] == EMPTY) {
                        if (!isSquareAttacked(board, 60, enemyColor) && 
                            !isSquareAttacked(board, 59, enemyColor) && 
                            !isSquareAttacked(board, 58, enemyColor)) {
                            moves.push_back(Move(60, 58, Flag_Castling));
                        }
                    }
                }
            }
        }

        // ==========================================
        // PART 3: KNIGHT MOVES
        // ==========================================
        bool isKnight = (piece == W_KNIGHT || piece == B_KNIGHT);
        if (isKnight) {
            int numMoves = NumKnightMoves[square];
            for (int i = 0; i < numMoves; i++) {
                int targetSquare = KnightMoves[square][i];
                int pieceAtTarget = board.squares[targetSquare];

                if (pieceAtTarget == EMPTY || isEnemy(pieceAtTarget, friendlyColor)) {
                    moves.push_back(Move(square, targetSquare));
                }
            }
        }

        // ==========================================
        // PART 4: PAWN MOVES
        // ==========================================
        bool isPawn = (piece == W_PAWN || piece == B_PAWN);
        if (isPawn) {
            int direction = (friendlyColor == WHITE) ? 8 : -8;
            int startRank = (friendlyColor == WHITE) ? 1 : 6;
            int promoRank = (friendlyColor == WHITE) ? 7 : 0;
            
            int currentRank = square / 8;
            int currentFile = square % 8;

            int singlePush = square + direction;
            if (board.squares[singlePush] == EMPTY) {
                int targetRank = singlePush / 8;
                if (targetRank == promoRank) {
                    int promoOffset = (friendlyColor == WHITE) ? 0 : 6;
                    moves.push_back(Move(square, singlePush, Flag_Promotion, W_QUEEN + promoOffset));
                    moves.push_back(Move(square, singlePush, Flag_Promotion, W_ROOK + promoOffset));
                    moves.push_back(Move(square, singlePush, Flag_Promotion, W_BISHOP + promoOffset));
                    moves.push_back(Move(square, singlePush, Flag_Promotion, W_KNIGHT + promoOffset));
                } else {
                    moves.push_back(Move(square, singlePush));
                    
                    if (currentRank == startRank) {
                        int doublePush = square + (direction * 2);
                        if (board.squares[doublePush] == EMPTY) {
                            moves.push_back(Move(square, doublePush, Flag_PawnDoublePush));
                        }
                    }
                }
            }

            int captureOffsets[2] = {direction - 1, direction + 1};
            for (int i = 0; i < 2; i++) {
                int targetSquare = square + captureOffsets[i];
                if ((i == 0 && currentFile == 0) || (i == 1 && currentFile == 7)) continue;
                
                int targetRank = targetSquare / 8;
                int pieceAtTarget = board.squares[targetSquare];

                if (isEnemy(pieceAtTarget, friendlyColor) || targetSquare == board.enPassantSquare) {
                    int moveFlag = (targetSquare == board.enPassantSquare) ? Flag_EnPassant : Flag_None;

                    if (targetRank == promoRank) {
                        int promoOffset = (friendlyColor == WHITE) ? 0 : 6;
                        moves.push_back(Move(square, targetSquare, Flag_Promotion, W_QUEEN + promoOffset));
                        moves.push_back(Move(square, targetSquare, Flag_Promotion, W_ROOK + promoOffset));
                        moves.push_back(Move(square, targetSquare, Flag_Promotion, W_BISHOP + promoOffset));
                        moves.push_back(Move(square, targetSquare, Flag_Promotion, W_KNIGHT + promoOffset));
                    } else {
                        moves.push_back(Move(square, targetSquare, moveFlag));
                    }
                }
            }
        }
    } 

    return moves;
}

inline std::string moveToUCI(Move move) {
    std::string uci = "";
    uci += (char)('a' + (move.fromSquare % 8));
    uci += (char)('1' + (move.fromSquare / 8));
    uci += (char)('a' + (move.toSquare % 8));
    uci += (char)('1' + (move.toSquare / 8));
    if (move.promotedPiece != EMPTY) {
        if (move.promotedPiece == W_QUEEN || move.promotedPiece == B_QUEEN) uci += 'q';
        else if (move.promotedPiece == W_ROOK || move.promotedPiece == B_ROOK) uci += 'r';
        else if (move.promotedPiece == W_BISHOP || move.promotedPiece == B_BISHOP) uci += 'b';
        else if (move.promotedPiece == W_KNIGHT || move.promotedPiece == B_KNIGHT) uci += 'n';
    }
    return uci;
}

#endif
