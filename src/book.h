#ifndef BOOK_H
#define BOOK_H

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "polyglot_keys.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>

// Polyglot files are Big-Endian. We must flip the bytes for our system.
inline uint64_t swapEndian64(uint64_t val) { return __builtin_bswap64(val); }
inline uint16_t swapEndian16(uint16_t val) { return __builtin_bswap16(val); }

#pragma pack(push, 1) // Ensure no padding issues when reading binary chunks
struct PolyglotEntry {
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;
};
#pragma pack(pop)

// ---------------------------------------------------------
// RAM CACHE FOR THE BOOK
// ---------------------------------------------------------

// We use a static local variable to safely store the book globally in RAM 
// without causing linker errors across multiple files.
inline std::vector<PolyglotEntry>& getBookMemory() {
    static std::vector<PolyglotEntry> book;
    return book;
}

// Call this ONCE in your main() or engine init() function!
inline void loadOpeningBook(const std::string& bookPath) {
    std::ifstream file(bookPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "info string Warning: Opening book not found at " << bookPath << std::endl;
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    size_t numEntries = fileSize / sizeof(PolyglotEntry);
    auto& book = getBookMemory();
    book.resize(numEntries);
    
    // Read the entire 30MB file into RAM at once (Lightning fast!)
    file.read(reinterpret_cast<char*>(book.data()), fileSize);
    file.close();

    // Pre-process endianness ONCE so we don't have to do it during the game
    for (auto& entry : book) {
        entry.key = swapEndian64(entry.key);
        entry.move = swapEndian16(entry.move);
        entry.weight = swapEndian16(entry.weight);
    }

    std::cout << "info string Loaded " << numEntries << " book entries into RAM." << std::endl;
}

// ---------------------------------------------------------
// HASHING & LOOKUP LOGIC
// ---------------------------------------------------------

// FIX 1: Map Chessboi's pieces to Polyglot's STRICT interleaved index system
inline int getPolyglotPiece(int piece) {
    switch (piece) {
        case B_PAWN:   return 0;
        case W_PAWN:   return 1;
        case B_KNIGHT: return 2;
        case W_KNIGHT: return 3;
        case B_BISHOP: return 4;
        case W_BISHOP: return 5;
        case B_ROOK:   return 6;
        case W_ROOK:   return 7;
        case B_QUEEN:  return 8;
        case W_QUEEN:  return 9;
        case B_KING:   return 10;
        case W_KING:   return 11;
        default:       return -1;
    }
}

// Generate the EXACT hash Polyglot expects
inline uint64_t computePolyglotHash(const BoardState& board) {
    uint64_t hash = 0;

    // 1. Hash Pieces
    for (int sq = 0; sq < 64; sq++) {
        int piece = board.squares[sq];
        if (piece != EMPTY) {
            int polyPiece = getPolyglotPiece(piece);
            if (polyPiece != -1) {
                hash ^= PolyglotRandom64[64 * polyPiece + sq];
            }
        }
    }

    // 2. Hash Castling
    if (board.whiteCastleK) hash ^= PolyglotRandom64[768];
    if (board.whiteCastleQ) hash ^= PolyglotRandom64[769];
    if (board.blackCastleK) hash ^= PolyglotRandom64[770];
    if (board.blackCastleQ) hash ^= PolyglotRandom64[771];

    // 3. Hash En Passant (Strict Polyglot Rule: Only hash if WE have an adjacent pawn to capture it!)
    if (board.enPassantSquare != -1) {
        int epFile = board.enPassantSquare % 8;
        int pawnRank = (board.sideToMove == WHITE) ? 4 : 3; 
        
        int leftSq = pawnRank * 8 + epFile - 1;
        int rightSq = pawnRank * 8 + epFile + 1;
        
        // FIX 2: We need to check if OUR pawn is sitting next to it!
        int myPawn = (board.sideToMove == WHITE) ? W_PAWN : B_PAWN; 
        
        bool adjacentEnemy = false;
        if (epFile > 0 && board.squares[leftSq] == myPawn) adjacentEnemy = true;
        if (epFile < 7 && board.squares[rightSq] == myPawn) adjacentEnemy = true;

        if (adjacentEnemy) {
            hash ^= PolyglotRandom64[772 + epFile];
        }
    }

    // 4. Hash Turn (Polyglot only hashes White's turn)
    if (board.sideToMove == WHITE) {
        hash ^= PolyglotRandom64[780];
    }

    return hash;
}

// Search RAM for a book move!
inline Move getBookMove(const BoardState& board) {
    const auto& book = getBookMemory();
    if (book.empty()) return Move(); // Book wasn't loaded

    uint64_t polyHash = computePolyglotHash(board);
    
    // RAM Binary Search (Takes ~0.00001 seconds)
    auto it = std::lower_bound(book.begin(), book.end(), polyHash,
        [](const PolyglotEntry& entry, uint64_t hash) {
            return entry.key < hash;
        });

    // If we didn't find the hash
    if (it == book.end() || it->key != polyHash) {
        return Move(); 
    }

    // Collect all valid moves for this hash
    std::vector<PolyglotEntry> validMoves;
    int totalWeight = 0;
    
    while (it != book.end() && it->key == polyHash) {
        validMoves.push_back(*it);
        totalWeight += it->weight;
        ++it;
    }

    if (validMoves.empty() || totalWeight == 0) return Move();

    // Pick a random move based on weight
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, totalWeight - 1);
    int randomChoice = dist(gen);
    
    int runningWeight = 0;
    uint16_t chosenPolyMove = validMoves[0].move;
    for (const auto& entry : validMoves) {
        runningWeight += entry.weight;
        if (randomChoice < runningWeight) {
            chosenPolyMove = entry.move;
            break;
        }
    }

    // --- TRANSLATE POLYGLOT TO ENGINE ---
    int poly_to = chosenPolyMove & 63;
    int poly_from = (chosenPolyMove >> 6) & 63;
    int poly_promo = (chosenPolyMove >> 12) & 7;

    // 1. Remap Castling (Polyglot: King takes Rook -> Engine: King moves 2 squares)
    if (board.squares[poly_from] == W_KING) {
        if (poly_from == 4 && poly_to == 7) poly_to = 6; // e1h1 -> e1g1
        if (poly_from == 4 && poly_to == 0) poly_to = 2; // e1a1 -> e1c1
    } else if (board.squares[poly_from] == B_KING) {
        if (poly_from == 60 && poly_to == 63) poly_to = 62; // e8h8 -> e8g8
        if (poly_from == 60 && poly_to == 56) poly_to = 58; // e8a8 -> e8c8
    }

    // 2. Remap Promotions
    int engine_promo = 0;
    if (poly_promo != 0) {
        int offset = (board.sideToMove == WHITE) ? 0 : 6;
        if (poly_promo == 1) engine_promo = W_KNIGHT + offset;
        else if (poly_promo == 2) engine_promo = W_BISHOP + offset;
        else if (poly_promo == 3) engine_promo = W_ROOK + offset;
        else if (poly_promo == 4) engine_promo = W_QUEEN + offset;
    }

    // 3. Find the matching fully-flagged move!
    std::vector<Move> legalMoves = generateMoves(board);
    for (Move m : legalMoves) {
        if (m.fromSquare == poly_from && m.toSquare == poly_to) {
            if (poly_promo == 0 || m.promotedPiece == engine_promo) {
                return m; // Found it!
            }
        }
    }

    return Move(); 
}

#endif
