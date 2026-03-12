#ifndef BOARD_H
#define BOARD_H

#include "constants.h"
#include <string>
#include <cctype>
#include <cstdint> // Needed for uint64_t

// --- V1.3 ZOBRIST HASHING SETUP ---
// A super-fast pseudo-random number generator
inline uint64_t random64() {
    static uint64_t seed = 0x98f107; // Engine's "DNA" seed
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    return seed * 2685821657736338717LL;
}

// The Zobrist Arrays
inline uint64_t pieceKeys[13][64]; // 12 pieces + EMPTY, 64 squares
inline uint64_t sideKey;           // Who's turn is it?
inline uint64_t castleKeys[16];    // 16 possible castling right combinations
inline uint64_t epKeys[64];        // 64 possible en passant squares
inline bool zobristInitialized = false;

inline void initZobrist() {
    if (zobristInitialized) return;
    for (int p = 0; p < 13; p++) {
        for (int s = 0; s < 64; s++) pieceKeys[p][s] = random64();
    }
    sideKey = random64();
    for (int i = 0; i < 16; i++) castleKeys[i] = random64();
    for (int i = 0; i < 64; i++) epKeys[i] = random64();
    zobristInitialized = true;
}


// --- 1. DEFINE THE STRUCT ---
struct BoardState {
    int squares[64]; 
    int sideToMove;
    bool whiteCastleK;
    bool whiteCastleQ;
    bool blackCastleK;
    bool blackCastleQ;
    int enPassantSquare;

    // --- NEW: THE ENGINE'S MEMORY ---
    uint64_t hashKey;          // The current 64-bit ID of the board
    uint64_t history[2048];    // Every board ID that has happened this game
    int ply;                   // How many half-moves have been played total
    int halfMoveClock;         // Half-moves since last pawn push or capture
};

// Helper function to turn the 4 castling booleans into a single number (0-15)
inline int getCastleRights(const BoardState& board) {
    int castle = 0;
    if (board.whiteCastleK) castle |= 1;
    if (board.whiteCastleQ) castle |= 2;
    if (board.blackCastleK) castle |= 4;
    if (board.blackCastleQ) castle |= 8;
    return castle;
}

// Generate the Hash from scratch (Only used when loading a FEN!)
inline uint64_t generateHashFromScratch(const BoardState& board) {
    uint64_t hash = 0;
    for (int i = 0; i < 64; i++) {
        int piece = board.squares[i];
        if (piece != EMPTY) hash ^= pieceKeys[piece][i]; // The Magic XOR!
    }
    if (board.sideToMove == BLACK) hash ^= sideKey;
    hash ^= castleKeys[getCastleRights(board)];
    if (board.enPassantSquare != -1) hash ^= epKeys[board.enPassantSquare];
    return hash;
}


// --- 2. LOAD FEN ---
inline void loadFEN(BoardState& board, std::string fen) {
    initZobrist(); // Make sure random numbers exist!

    for (int i = 0; i < 64; i++) board.squares[i] = EMPTY;
    int rank = 7, file = 0, i = 0;

    while (fen[i] != ' ' && i < fen.length()) {
        char c = fen[i];
        if (c == '/') { rank--; file = 0; }
        else if (isdigit(c)) { file += (c - '0'); }
        else {
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
    i++; 

    if (i < fen.length()) {
        board.sideToMove = (fen[i] == 'w') ? WHITE : BLACK;
        i += 2; 
    }

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

    // Set up the memory banks for move 1!
    board.ply = 0;
    board.halfMoveClock = 0;
    board.hashKey = generateHashFromScratch(board);
    board.history[board.ply] = board.hashKey;
}

// --- NEW: THE THREEFOLD DRAW DETECTOR ---
inline bool isRepetition(const BoardState& board) {
    // We only look backwards until the last pawn move or capture
    // (Because a repetition cannot cross a pawn move/capture)
    int limit = board.ply - board.halfMoveClock;
    if (limit < 0) limit = 0;
    
    // Step backwards by 2 plies (Looking only at the current player's turns)
    for (int i = board.ply - 2; i >= limit; i -= 2) {
        if (board.history[i] == board.hashKey) {
            return true; // We've seen this exact board before. ABORT!
        }
    }
    return false;
}

#endif
