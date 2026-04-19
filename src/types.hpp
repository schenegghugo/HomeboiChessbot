#pragma once
#include <cstdint>
#include <string>

// ==========================================
// 1. FUNDAMENTAL TYPES & CONSTANTS
// ==========================================
typedef uint64_t U64;

constexpr int MAX_PLY = 64;
constexpr int MAX_MOVES = 256; // Max theoretical moves in a single chess position

// ==========================================
// 2. ENUMS
// ==========================================
enum Color { 
    WHITE = 0, 
    BLACK = 1, 
    BOTH = 2 
};

// Piece Types (Regardless of color)
enum PieceType { 
    PAWN = 0, KNIGHT, BISHOP, ROOK, QUEEN, KING 
};

// Specific Pieces (Maps exactly to our NNUE array indices)
enum Piece { 
    wP = 0, wN, wB, wR, wQ, wK, 
    bP, bN, bB, bR, bQ, bK, 
    EMPTY = -1 
};

// Board Ranks (Horizontal rows)
enum Rank { 
    RANK_1 = 0, RANK_2, RANK_3, RANK_4, 
    RANK_5, RANK_6, RANK_7, RANK_8 
};

// Board Files (Vertical columns)
enum File { 
    FILE_A = 0, FILE_B, FILE_C, FILE_D, 
    FILE_E, FILE_F, FILE_G, FILE_H 
};

// Squares (0 to 63)
enum Square {
    a1 = 0, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
    SQ_NONE = 64
};

// Directions for sliding pieces and pawn pushes
enum Direction {
    NORTH =  8, EAST  =  1, SOUTH = -8, WEST  = -1,
    NORTH_EAST =  9, NORTH_WEST =  7,
    SOUTH_EAST = -7, SOUTH_WEST = -9
};

// Castling Rights Bitmask Flags
// 1 = wk, 2 = wq, 4 = bk, 8 = bq
enum CastlingRights {
    NO_CASTLING = 0,
    WK_CASTLING = 1, WQ_CASTLING = 2,
    BK_CASTLING = 4, BQ_CASTLING = 8,
    ALL_CASTLING = 15
};

// ==========================================
// 3. THE MOVE STRUCTURE
// ==========================================
struct Move {
    int piece;
    int from;
    int to;
    int captured; 
    int promoted; 
    bool is_castling;

    // Default constructor for empty moves (like TT initialization)
    Move() : piece(EMPTY), from(0), to(0), captured(EMPTY), promoted(EMPTY), is_castling(false) {}
    
    Move(int p, int f, int t, int c = EMPTY, int pr = EMPTY, bool castl = false) 
        : piece(p), from(f), to(t), captured(c), promoted(pr), is_castling(castl) {}

    // Equality operator for Killer/TT move matching
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to && promoted == other.promoted;
    }
    bool operator!=(const Move& other) const {
        return !(*this == other);
    }
};

// ==========================================
// 4. INLINE HELPER FUNCTIONS
// ==========================================

// Get the Color of a specific piece
inline constexpr Color color_of(int piece) {
    return (piece < 6) ? WHITE : BLACK;
}

// Get the PieceType (e.g., wN -> KNIGHT)
inline constexpr PieceType type_of(int piece) {
    return static_cast<PieceType>(piece % 6);
}

// Combine Color and PieceType into a Piece
inline constexpr Piece make_piece(Color c, PieceType pt) {
    return static_cast<Piece>((c * 6) + pt);
}

// Get the Rank (row) of a square
inline constexpr Rank rank_of(int sq) {
    return static_cast<Rank>(sq / 8);
}

// Get the File (column) of a square
inline constexpr File file_of(int sq) {
    return static_cast<File>(sq % 8);
}

// Combine File and Rank into a Square
inline constexpr Square make_square(File f, Rank r) {
    return static_cast<Square>((r * 8) + f);
}

// Flip a color (WHITE -> BLACK, BLACK -> WHITE)
inline constexpr Color flip_color(Color c) {
    return static_cast<Color>(c ^ 1);
}

// Flips the board vertically for Black's NNUE perspective (Rank 1 -> Rank 8)
inline constexpr int flip_sq(int sq) {
    return sq ^ 56;
}

// Swaps White pieces for Black pieces and vice-versa
inline constexpr int flip_piece(int piece) {
    return (piece < 6) ? piece + 6 : piece - 6;
}

// Helper to convert FEN characters to our Piece enum
inline int char_to_piece(char c) {
    switch (c) {
        case 'P': return wP; case 'N': return wN; case 'B': return wB; 
        case 'R': return wR; case 'Q': return wQ; case 'K': return wK;
        case 'p': return bP; case 'n': return bN; case 'b': return bB; 
        case 'r': return bR; case 'q': return bQ; case 'k': return bK;
        default: return EMPTY;
    }
}
