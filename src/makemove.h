#ifndef MAKEMOVE_H
#define MAKEMOVE_H

#include "board.h"
#include "move.h"

struct UndoData {
    int capturedPiece;
    int enPassantSquare;
    bool whiteCastleK;
    bool whiteCastleQ;
    bool blackCastleK;
    bool blackCastleQ;
};

inline UndoData makeMove(BoardState& board, Move move) {
    UndoData undo;

    // 1. Snapshot for undoing
    undo.capturedPiece   = board.squares[move.toSquare];
    undo.enPassantSquare = board.enPassantSquare;
    undo.whiteCastleK    = board.whiteCastleK;
    undo.whiteCastleQ    = board.whiteCastleQ;
    undo.blackCastleK    = board.blackCastleK;
    undo.blackCastleQ    = board.blackCastleQ;

    int piece = board.squares[move.fromSquare];

    // 2. Move the piece
    board.squares[move.toSquare] = piece;
    board.squares[move.fromSquare] = EMPTY;

    // 3. Weird chess rules handling
    if (move.flag == Flag_EnPassant) {
        int captureSquare = (board.sideToMove == WHITE) ? move.toSquare - 8 : move.toSquare + 8;
        undo.capturedPiece = board.squares[captureSquare]; 
        board.squares[captureSquare] = EMPTY;              
    }
    else if (move.flag == Flag_Promotion) {
        board.squares[move.toSquare] = move.promotedPiece;
    }
    else if (move.flag == Flag_Castling) {
        // TELEPORT THE ROOK!
        if (move.toSquare == 6) { board.squares[5] = W_ROOK; board.squares[7] = EMPTY; }       // White Kingside
        else if (move.toSquare == 2) { board.squares[3] = W_ROOK; board.squares[0] = EMPTY; }  // White Queenside
        else if (move.toSquare == 62) { board.squares[61] = B_ROOK; board.squares[63] = EMPTY; } // Black Kingside
        else if (move.toSquare == 58) { board.squares[59] = B_ROOK; board.squares[56] = EMPTY; } // Black Queenside
    }

    // 4. Update En Passant
    if (move.flag == Flag_PawnDoublePush) {
        board.enPassantSquare = (board.sideToMove == WHITE) ? move.fromSquare + 8 : move.fromSquare - 8;
    } else {
        board.enPassantSquare = -1;
    }

    // 5. UPDATE CASTLING RIGHTS
    // If King moves, lose both rights
    if (piece == W_KING) { board.whiteCastleK = false; board.whiteCastleQ = false; }
    if (piece == B_KING) { board.blackCastleK = false; board.blackCastleQ = false; }
    
    // If Rook moves, lose that side's right
    if (piece == W_ROOK) {
        if (move.fromSquare == 0) board.whiteCastleQ = false; // a1
        if (move.fromSquare == 7) board.whiteCastleK = false; // h1
    }
    if (piece == B_ROOK) {
        if (move.fromSquare == 56) board.blackCastleQ = false; // a8
        if (move.fromSquare == 63) board.blackCastleK = false; // h8
    }

    // If a Rook is CAPTURED, the defender loses that side's right
    if (undo.capturedPiece == W_ROOK) {
        if (move.toSquare == 0) board.whiteCastleQ = false;
        if (move.toSquare == 7) board.whiteCastleK = false;
    }
    if (undo.capturedPiece == B_ROOK) {
        if (move.toSquare == 56) board.blackCastleQ = false;
        if (move.toSquare == 63) board.blackCastleK = false;
    }

    // 6. Flip turn
    board.sideToMove = (board.sideToMove == WHITE) ? BLACK : WHITE;

    return undo;
}

inline void undoMove(BoardState& board, Move move, UndoData undo) {
    board.sideToMove = (board.sideToMove == WHITE) ? BLACK : WHITE;

    int piece = board.squares[move.toSquare];
    board.squares[move.fromSquare] = piece;
    board.squares[move.toSquare] = undo.capturedPiece;

    if (move.flag == Flag_EnPassant) {
        board.squares[move.toSquare] = EMPTY;
        int captureSquare = (board.sideToMove == WHITE) ? move.toSquare - 8 : move.toSquare + 8;
        board.squares[captureSquare] = undo.capturedPiece;
    }
    else if (move.flag == Flag_Promotion) {
        board.squares[move.fromSquare] = (board.sideToMove == WHITE) ? W_PAWN : B_PAWN;
    }
    else if (move.flag == Flag_Castling) {
        // Put the rook back where it started
        if (move.toSquare == 6) { board.squares[7] = W_ROOK; board.squares[5] = EMPTY; }       // White Kingside
        else if (move.toSquare == 2) { board.squares[0] = W_ROOK; board.squares[3] = EMPTY; }  // White Queenside
        else if (move.toSquare == 62) { board.squares[63] = B_ROOK; board.squares[61] = EMPTY; } // Black Kingside
        else if (move.toSquare == 58) { board.squares[56] = B_ROOK; board.squares[59] = EMPTY; } // Black Queenside
    }

    board.enPassantSquare = undo.enPassantSquare;
    board.whiteCastleK    = undo.whiteCastleK;
    board.whiteCastleQ    = undo.whiteCastleQ;
    board.blackCastleK    = undo.blackCastleK;
    board.blackCastleQ    = undo.blackCastleQ;
}

#endif
