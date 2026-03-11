#ifndef PRECOMPUTED_H
#define PRECOMPUTED_H

#include <algorithm> // For std::min 
#include <cmath>     // For std::abs

// The 8 directions a sliding piece can move considering the board defined as a 1D array from 0 to 63. 
// [0] == a1 and [63] == h8.
// N = +8, S = -8, E = +1, W = -1, NE = +9, NW = +7, SE = -7, SW = -9
const int DirectionOffsets[8] = {8, -8, -1, 1, 7, -7, 9, -9};

// A 2D array. For all 64 squares, how many steps to the edge in all 8 directions?
extern int NumSquaresToEdge[64][8];
extern int NumKnightMoves[64];      // Precompute knightMoves. The whole goal is to avoid modulo bottlenecks.
extern int KnightMoves[64][8];      // Array for the knightMoves.

// Function to calculate this map once when the program boots up
inline void precomputeData() {
    
    // --- 1. PRECOMPUTE SLIDING PIECE EDGES ---
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            
            int numNorth = 7 - rank;
            int numSouth = rank;
            int numWest  = file;
            int numEast  = 7 - file;

            int squareIndex = rank * 8 + file;

            NumSquaresToEdge[squareIndex][0] = numNorth;
            NumSquaresToEdge[squareIndex][1] = numSouth;
            NumSquaresToEdge[squareIndex][2] = numWest;
            NumSquaresToEdge[squareIndex][3] = numEast;
            
            NumSquaresToEdge[squareIndex][4] = std::min(numNorth, numWest); // NW
            NumSquaresToEdge[squareIndex][5] = std::min(numSouth, numEast); // SE
            NumSquaresToEdge[squareIndex][6] = std::min(numNorth, numEast); // NE
            NumSquaresToEdge[squareIndex][7] = std::min(numSouth, numWest); // SW
        }
    }

    // --- 2. PRECOMPUTE KNIGHT MOVES ---
    const int knightJumps[8] = {15, 17, -17, -15, 10, -6, 6, -10};

    for (int square = 0; square < 64; square++) {
        int validJumps = 0;
        int startFile = square % 8;

        for (int i = 0; i < 8; i++) {
            int targetSquare = square + knightJumps[i];

            // Is it on the board?
            if (targetSquare >= 0 && targetSquare < 64) {
                int targetFile = targetSquare % 8;

                // Did it wrap around the edge?
                if (std::abs(startFile - targetFile) <= 2) {
                    // It's a valid mathematical jump! Save it.
                    KnightMoves[square][validJumps] = targetSquare;
                    validJumps++;
                }
            }
        }
        // Save how many valid jumps we found for this square
        NumKnightMoves[square] = validJumps;
    }
}

#endif
