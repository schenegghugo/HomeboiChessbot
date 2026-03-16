#pragma once
#include <cstdint>
#include <string>

// --- Move Flags ---
constexpr int QUIET_MOVE       = 0;
constexpr int DOUBLE_PUSH      = 1;
constexpr int KING_CASTLE      = 2;
constexpr int QUEEN_CASTLE     = 3;
constexpr int CAPTURE          = 4;
constexpr int EN_PASSANT       = 5;
// 6 and 7 are unused, but could be reserved
constexpr int PROMOTE_KNIGHT   = 8;
constexpr int PROMOTE_BISHOP   = 9;
constexpr int PROMOTE_ROOK     = 10;
constexpr int PROMOTE_QUEEN    = 11;
constexpr int PROMOTE_K_CAP    = 12; // Promotion + Capture
constexpr int PROMOTE_B_CAP    = 13;
constexpr int PROMOTE_R_CAP    = 14;
constexpr int PROMOTE_Q_CAP    = 15;

class Move {
public:
    uint16_t value; // The single 16-bit integer holding EVERYTHING

    // Constructors
    Move() : value(0) {}
    Move(uint16_t v) : value(v) {}
    
    // Encode the move!
    Move(int source, int target, int flag = QUIET_MOVE) {
        value = (source & 0x3F) | ((target & 0x3F) << 6) | ((flag & 0xF) << 12);
    }

    // Decode the move using Bitwise Masks
    int source() const { return value & 0x3F; }                 // Extract first 6 bits
    int target() const { return (value >> 6) & 0x3F; }          // Shift right 6, extract next 6
    int flag() const   { return (value >> 12) & 0xF; }          // Shift right 12, extract last 4

    // Helper Boolean Functions
    bool is_capture() const { 
        int f = flag(); 
        return f == CAPTURE || f == EN_PASSANT || f >= PROMOTE_K_CAP; 
    }
    
    bool is_promotion() const { 
        return flag() >= PROMOTE_KNIGHT; 
    }

    // Convert to Universal Chess Interface (UCI) string format (e.g. "e2e4", "e7e8q")
    std::string to_string() const {
        std::string files = "abcdefgh";
        std::string ranks = "12345678";
        
        std::string s = "";
        s += files[source() % 8];
        s += ranks[source() / 8];
        s += files[target() % 8];
        s += ranks[target() / 8];
        
        // Add promotion character if applicable
        int f = flag();
        if (f >= PROMOTE_KNIGHT) {
            if (f == PROMOTE_KNIGHT || f == PROMOTE_K_CAP) s += 'n';
            if (f == PROMOTE_BISHOP || f == PROMOTE_B_CAP) s += 'b';
            if (f == PROMOTE_ROOK   || f == PROMOTE_R_CAP) s += 'r';
            if (f == PROMOTE_QUEEN  || f == PROMOTE_Q_CAP) s += 'q';
        }
        return s;
    }
};

class MoveList {
public:
    Move moves[256]; // Max possible chess moves in a position is ~218
    int count = 0;

    void add(Move move) {
        moves[count++] = move;
    }
};
