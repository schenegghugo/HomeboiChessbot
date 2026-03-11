#ifndef BOARD_H
#define BOARD_H

#include "constants.h"
#include <string>
#include <cctype>

// 1. DEFINE THE STRUCT FIRST
struct BoardState {
    int squares[64]; 
    int sideToMove;
    bool whiteCastleK;
    bool whiteCastleQ;
    bool blackCastleK;
    bool blackCastleQ;
    int enPassantSquare;
};

// 2. NOW WE CAN USE IT IN FUNCTIONS
inline void loadFEN(BoardState& board, std::string fen) {
    // 1. Clear the board
    for (int i = 0; i < 64; i++) board.squares[i] = EMPTY;
    
    int rank = 7;
    int file = 0;
    int i = 0;

    // 2. Parse the pieces
    while (fen[i] != ' ' && i < fen.length()) {
        char c = fen[i];
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += (c - '0');
        } else {
            int piece = EMPTY;
            if (c == 'P') piece = W_PAWN;   else if (c == 'p') piece = B_PAWN;
            else if (c == 'N') piece = W_KNIGHT; else if (c == 'n') piece = B_KNIGHT;
            else if (c == 'B') piece = W_BISHOP; else if (c == 'b') piece = B_BISHOP;
            else if (c == 'R') piece = W_ROOK;   else if (c == 'r') piece = B_ROOK;
            else if (c == 'Q') piece = W_QUEEN;  else if (c == 'q') piece = B_QUEEN;
            else if (c == 'K') piece = W_KING;   else if (c == 'k') piece = B_KING;

            board.squares[rank * 8 + file] = piece;
            file++;
        }
        i++;
    }

    i++; // Skip the space

    // 3. Whose turn is it?
    if (i < fen.length()) {
        board.sideToMove = (fen[i] == 'w') ? WHITE : BLACK;
        i += 2; // Skip 'w ' or 'b '
    }

    // 4. Castling rights
    board.whiteCastleK = false; board.whiteCastleQ = false;
    board.blackCastleK = false; board.blackCastleQ = false;
    
    while (i < fen.length() && fen[i] != ' ') {
        if (fen[i] == 'K') board.whiteCastleK = true;
        if (fen[i] == 'Q') board.whiteCastleQ = true;
        if (fen[i] == 'k') board.blackCastleK = true;
        if (fen[i] == 'q') board.blackCastleQ = true;
        if (fen[i] == '-') break;
        i++;
    }

    board.enPassantSquare = -1;
}

#endif
