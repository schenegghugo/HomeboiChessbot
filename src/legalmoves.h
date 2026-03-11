#ifndef LEGALMOVES_H
#define LEGALMOVES_H

#include <vector>
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "makemove.h"

inline std::vector<Move> getLegalMoves(BoardState& board) {
    std::vector<Move> pseudoMoves = generateMoves(board);
    std::vector<Move> legalMoves;
    
    int friendlyColor = board.sideToMove;
    int enemyColor = (friendlyColor == WHITE) ? BLACK : WHITE;
    int kingPiece = (friendlyColor == WHITE) ? W_KING : B_KING;

    for (Move move : pseudoMoves) {
        // 1. Jump forward in time to test the move
        UndoData undo = makeMove(board, move);
        
        // 2. Find where our King is sitting NOW
        int kingSquare = -1;
        for (int i = 0; i < 64; i++) {
            if (board.squares[i] == kingPiece) {
                kingSquare = i;
                break;
            }
        }

        // 3. If the King is NOT attacked, the move is perfectly legal!
        if (kingSquare != -1 && !isSquareAttacked(board, kingSquare, enemyColor)) {
            legalMoves.push_back(move);
        }
        
        // 4. Rewind time so we don't mess up the actual game
        undoMove(board, move, undo);
    }
    
    return legalMoves;
}

#endif
