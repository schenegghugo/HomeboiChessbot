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
    
    // Memory backups!
    uint64_t hashKey;
    int halfMoveClock;
};

inline UndoData makeMove(BoardState& board, Move move) {
    UndoData undo;

    // 1. Snapshot the physical board AND the Memory
    undo.capturedPiece   = board.squares[move.toSquare];
    undo.enPassantSquare = board.enPassantSquare;
    undo.whiteCastleK    = board.whiteCastleK;
    undo.whiteCastleQ    = board.whiteCastleQ;
    undo.blackCastleK    = board.blackCastleK;
    undo.blackCastleQ    = board.blackCastleQ;
    undo.hashKey         = board.hashKey;
    undo.halfMoveClock   = board.halfMoveClock;

    int piece = board.squares[move.fromSquare];

    // --- INCREMENTAL HASHING: REMOVE THE OLD BOARD STATE ---
    uint64_t hash = board.hashKey;
    if (board.sideToMove == BLACK) hash ^= sideKey;
    if (board.enPassantSquare != -1) hash ^= epKeys[board.enPassantSquare];
    hash ^= castleKeys[getCastleRights(board)];
    hash ^= pieceKeys[piece][move.fromSquare]; // Pick up the piece

    // 2. Move the piece physically
    board.squares[move.toSquare] = piece;
    board.squares[move.fromSquare] = EMPTY;

    // 3. Weird chess rules handling & Hash updating
    if (move.flag == Flag_EnPassant) {
        int captureSquare = (board.sideToMove == WHITE) ? move.toSquare - 8 : move.toSquare + 8;
        undo.capturedPiece = board.squares[captureSquare]; 
        board.squares[captureSquare] = EMPTY; 
        hash ^= pieceKeys[undo.capturedPiece][captureSquare]; // Remove captured pawn from memory             
    }
    else if (move.flag == Flag_Promotion) {
        board.squares[move.toSquare] = move.promotedPiece;
    }
    else if (move.flag == Flag_Castling) {
        // TELEPORT THE ROOK (and update the hash!)
        if (move.toSquare == 6) { 
            board.squares[5] = W_ROOK; board.squares[7] = EMPTY; 
            hash ^= pieceKeys[W_ROOK][7]; hash ^= pieceKeys[W_ROOK][5];
        } else if (move.toSquare == 2) { 
            board.squares[3] = W_ROOK; board.squares[0] = EMPTY; 
            hash ^= pieceKeys[W_ROOK][0]; hash ^= pieceKeys[W_ROOK][3];
        } else if (move.toSquare == 62) { 
            board.squares[61] = B_ROOK; board.squares[63] = EMPTY; 
            hash ^= pieceKeys[B_ROOK][63]; hash ^= pieceKeys[B_ROOK][61];
        } else if (move.toSquare == 58) { 
            board.squares[59] = B_ROOK; board.squares[56] = EMPTY; 
            hash ^= pieceKeys[B_ROOK][56]; hash ^= pieceKeys[B_ROOK][59];
        }
    }

    // Hash out standard captures
    if (undo.capturedPiece != EMPTY && move.flag != Flag_EnPassant) {
        hash ^= pieceKeys[undo.capturedPiece][move.toSquare];
    }

    // Hash in the new piece location
    int movedPiece = (move.flag == Flag_Promotion) ? move.promotedPiece : piece;
    hash ^= pieceKeys[movedPiece][move.toSquare];

    // 4. Update En Passant
    if (move.flag == Flag_PawnDoublePush) {
        board.enPassantSquare = (board.sideToMove == WHITE) ? move.fromSquare + 8 : move.fromSquare - 8;
    } else {
        board.enPassantSquare = -1;
    }

    // 5. Update Castling Rights
    if (piece == W_KING) { board.whiteCastleK = false; board.whiteCastleQ = false; }
    if (piece == B_KING) { board.blackCastleK = false; board.blackCastleQ = false; }
    
    if (piece == W_ROOK) {
        if (move.fromSquare == 0) board.whiteCastleQ = false; 
        if (move.fromSquare == 7) board.whiteCastleK = false; 
    }
    if (piece == B_ROOK) {
        if (move.fromSquare == 56) board.blackCastleQ = false; 
        if (move.fromSquare == 63) board.blackCastleK = false; 
    }

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

    // --- INCREMENTAL HASHING: ADD THE NEW STATE ---
    if (board.sideToMove == BLACK) hash ^= sideKey;
    if (board.enPassantSquare != -1) hash ^= epKeys[board.enPassantSquare];
    hash ^= castleKeys[getCastleRights(board)];

    // 7. Save to Memory Banks
    board.ply++;
    board.halfMoveClock++;
    if (piece == W_PAWN || piece == B_PAWN || undo.capturedPiece != EMPTY) {
        board.halfMoveClock = 0; // Reset 50-move rule
    }
    
    board.hashKey = hash;
    board.history[board.ply] = hash; // Remember this board state!

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
        if (move.toSquare == 6) { board.squares[7] = W_ROOK; board.squares[5] = EMPTY; }       
        else if (move.toSquare == 2) { board.squares[0] = W_ROOK; board.squares[3] = EMPTY; }  
        else if (move.toSquare == 62) { board.squares[63] = B_ROOK; board.squares[61] = EMPTY; } 
        else if (move.toSquare == 58) { board.squares[56] = B_ROOK; board.squares[59] = EMPTY; } 
    }

    board.enPassantSquare = undo.enPassantSquare;
    board.whiteCastleK    = undo.whiteCastleK;
    board.whiteCastleQ    = undo.whiteCastleQ;
    board.blackCastleK    = undo.blackCastleK;
    board.blackCastleQ    = undo.blackCastleQ;

    // --- RESTORE MEMORY ---
    // Instead of doing reverse math, we just literally load the backup!
    board.ply--;
    board.hashKey = undo.hashKey;
    board.halfMoveClock = undo.halfMoveClock;
}

#endif
