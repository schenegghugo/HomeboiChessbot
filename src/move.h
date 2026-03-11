#ifndef MOVE_H
#define MOVE_H

// Flags to handle the weird rules of chess
enum MoveFlag {
    Flag_None = 0,
    Flag_EnPassant = 1,
    Flag_Castling = 2,
    Flag_Promotion = 3,
    Flag_PawnDoublePush = 4 // Tells the board to activate an En Passant target square
};

struct Move {
    int fromSquare;
    int toSquare;
    int flag;
    int promotedPiece; // Stores the piece we promote to (e.g., W_QUEEN), otherwise 0 (EMPTY)

    // Default constructor (Needed so C++ can create empty arrays of Moves)
    Move() {
        fromSquare = 0;
        toSquare = 0;
        flag = Flag_None;
        promotedPiece = 0;
    }

    // Quick constructor so we can create a move in one line of code:
    // Example: Move m = Move(12, 28, Flag_PawnDoublePush);
    Move(int from, int to, int f = Flag_None, int promo = 0) {
        fromSquare = from;
        toSquare = to;
        flag = f;
        promotedPiece = promo;
    }
};

#endif
