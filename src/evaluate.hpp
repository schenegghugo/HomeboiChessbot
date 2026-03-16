#pragma once
#include "board.hpp"
#include "movegen.hpp"
#include "magics.hpp"
#include <algorithm>

namespace Eval {

    // ==========================================
    // --- MATERIAL & POSITIONAL CONSTANTS ---
    // ==========================================
    const int PawnValue   = 100;
    const int KnightValue = 320;
    const int BishopValue = 330;
    const int RookValue   = 500;
    const int QueenValue  = 900;
    const int KingValue   = 100000;

    const int BISHOP_PAIR_MG = 30;
    const int BISHOP_PAIR_EG = 50;

    const int ISOLATED_PAWN_MG = -15;
    const int ISOLATED_PAWN_EG = -20;

    const int DOUBLED_PAWN_MG = -15;
    const int DOUBLED_PAWN_EG = -20;

    const int PHALANX_PAWN_MG = 10;
    const int PHALANX_PAWN_EG = 15;

    const int ROOK_OPEN_FILE_MG = 20;
    const int ROOK_OPEN_FILE_EG = 20;
    const int ROOK_SEMI_OPEN_MG = 10;
    const int ROOK_SEMI_OPEN_EG = 10;

    const int BISHOP_MOBILITY_MG = 3;
    const int BISHOP_MOBILITY_EG = 4;
    const int ROOK_MOBILITY_MG = 2;
    const int ROOK_MOBILITY_EG = 3;
    const int QUEEN_MOBILITY_MG = 1;
    const int QUEEN_MOBILITY_EG = 2;

    const int PASSED_PAWN_MG[8] = { 0, 5, 10, 20, 40, 70, 120, 0 };  
    const int PASSED_PAWN_EG[8] = { 0, 10, 25, 50, 90, 150, 250, 0 }; 

    constexpr uint64_t FILE_MASKS[8] = {
        0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
        0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL
    };

    // ==========================================
    // --- MIDGAME PIECE SQUARE TABLES ---
    // ==========================================
    const int mg_pawnPST[64] = {
          0,  0,  0,  0,  0,  0,  0,  0,
         50, 50, 50, 50, 50, 50, 50, 50,
         10, 10, 20, 30, 30, 20, 10, 10,
          5,  5, 10, 25, 25, 10,  5,  5,
          0,  0,  0, 20, 20,  0,  0,  0,
          5, -5,-10,  0,  0,-10, -5,  5,
          5, 10, 10,-20,-20, 10, 10,  5,
          0,  0,  0,  0,  0,  0,  0,  0
    };

    const int mg_knightPST[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };

    const int mg_bishopPST[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };

    const int mg_rookPST[64] = {
          0,  0,  0,  0,  0,  0,  0,  0,
          5, 10, 10, 10, 10, 10, 10,  5,
         -5,  0,  0,  0,  0,  0,  0, -5,
         -5,  0,  0,  0,  0,  0,  0, -5,
         -5,  0,  0,  0,  0,  0,  0, -5,
         -5,  0,  0,  0,  0,  0,  0, -5,
         -5,  0,  0,  0,  0,  0,  0, -5,
          0,  0,  0,  5,  5,  0,  0,  0
    };

    const int mg_queenPST[64] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };

    const int mg_kingPST[64] = {
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
         20, 30, 10,  0,  0, 10, 30, 20
    };

    // ==========================================
    // --- ENDGAME PIECE SQUARE TABLES ---
    // ==========================================
    const int eg_pawnPST[64] = {
          0,  0,  0,  0,  0,  0,  0,  0,
        120,120,120,120,120,120,120,120,
         80, 80, 80, 80, 80, 80, 80, 80,
         50, 50, 50, 50, 50, 50, 50, 50,
         30, 30, 30, 30, 30, 30, 30, 30,
         20, 20, 20, 20, 20, 20, 20, 20,
         10, 10, 10, 10, 10, 10, 10, 10,
          0,  0,  0,  0,  0,  0,  0,  0
    };

    const int eg_knightPST[64] = {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -50,-40,-30,-30,-30,-30,-40,-50
    };

    const int eg_bishopPST[64] = {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20
    };

    const int eg_rookPST[64] = {
         10, 10, 10, 10, 10, 10, 10, 10,
         20, 20, 20, 20, 20, 20, 20, 20,
          0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,
         -5, -5, -5, -5, -5, -5, -5, -5,
         -5, -5, -5, -5, -5, -5, -5, -5
    };

    const int eg_queenPST[64] = {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  5, 10, 10, 10, 10,  5,-10,
         -5,  5, 10, 20, 20, 10,  5, -5,
          0,  5, 10, 20, 20, 10,  5, -5,
        -10,  5, 10, 10, 10, 10,  5,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };

    const int eg_kingPST[64] = {
        -50,-30,-30,-30,-30,-30,-30,-50,
        -30,-30,  0,  0,  0,  0,-30,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 30, 40, 40, 30,-10,-30,
        -30,-10, 20, 30, 30, 20,-10,-30,
        -30,-20,-10,  0,  0,-10,-20,-30,
        -50,-40,-30,-20,-20,-30,-40,-50
    };

    // ==========================================
    // --- HIGH-SPEED BITBOARD HELPERS ---
    // ==========================================

    inline bool isPassedPawn(int sq, Color color, uint64_t enemy_pawns) {
        int file = sq & 7;
        int rank = sq >> 3;
        uint64_t mask = 0;
        
        // Build a mask of all squares in front of the pawn (including adjacent files)
        if (color == Color::White) {
            for (int r = rank + 1; r < 8; r++) {
                mask |= (1ULL << (r * 8 + file));
                if (file > 0) mask |= (1ULL << (r * 8 + file - 1));
                if (file < 7) mask |= (1ULL << (r * 8 + file + 1));
            }
        } else {
            for (int r = rank - 1; r >= 0; r--) {
                mask |= (1ULL << (r * 8 + file));
                if (file > 0) mask |= (1ULL << (r * 8 + file - 1));
                if (file < 7) mask |= (1ULL << (r * 8 + file + 1));
            }
        }
        return (mask & enemy_pawns) == 0; 
    }

    inline int evaluateKingSafety(int kingSq, Color color, uint64_t my_pawns, uint64_t opp_pawns) {
        int penalty = 0;
        int file = kingSq & 7;
        int rank = kingSq >> 3;

        if (color == Color::White && rank > 1) penalty -= 20; 
        if (color == Color::Black && rank < 6) penalty -= 20; 

        // Look at the 3 files surrounding the King
        for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); f++) {
            int friendlyPawns = __builtin_popcountll(my_pawns & FILE_MASKS[f]);
            int enemyPawns    = __builtin_popcountll(opp_pawns & FILE_MASKS[f]);

            if (friendlyPawns == 0) {
                penalty -= 15; // Missing pawn shield
                if (enemyPawns == 0) penalty -= 25; // Fully open file!
                else penalty -= 10; // Semi-open file
            }
        }
        return penalty;
    }

    // ==========================================
    // --- MAIN EVALUATION FUNCTION ---
    // ==========================================

    inline int evaluate(const Board& board) {
        int mgScore = 0;
        int egScore = 0;
        
        // --- Calculate Game Phase for Tapered Eval ---
        int knights = __builtin_popcountll(board.pieces[static_cast<int>(Color::White)][KNIGHT] | board.pieces[static_cast<int>(Color::Black)][KNIGHT]);
        int bishops = __builtin_popcountll(board.pieces[static_cast<int>(Color::White)][BISHOP] | board.pieces[static_cast<int>(Color::Black)][BISHOP]);
        int rooks   = __builtin_popcountll(board.pieces[static_cast<int>(Color::White)][ROOK]   | board.pieces[static_cast<int>(Color::Black)][ROOK]);
        int queens  = __builtin_popcountll(board.pieces[static_cast<int>(Color::White)][QUEEN]  | board.pieces[static_cast<int>(Color::Black)][QUEEN]);
        
        int gamePhase = (knights * 1) + (bishops * 1) + (rooks * 2) + (queens * 4);
        if (gamePhase > 24) gamePhase = 24;

        int wBishops = __builtin_popcountll(board.pieces[static_cast<int>(Color::White)][BISHOP]);
        int bBishops = __builtin_popcountll(board.pieces[static_cast<int>(Color::Black)][BISHOP]);
        if (wBishops >= 2) { mgScore += BISHOP_PAIR_MG; egScore += BISHOP_PAIR_EG; }
        if (bBishops >= 2) { mgScore -= BISHOP_PAIR_MG; egScore -= BISHOP_PAIR_EG; }

        uint64_t occ = board.occupancies[static_cast<int>(Color::Both)];

        // Loop through both colors
        for (int c = 0; c < 2; ++c) {
            Color col = static_cast<Color>(c);
            int sign = (col == Color::White) ? 1 : -1;
            uint64_t my_pawns = board.pieces[c][PAWN];
            uint64_t opp_pawns = board.pieces[c ^ 1][PAWN];

            // 1. PAWNS
            uint64_t bb = board.pieces[c][PAWN];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                int file = sq & 7;
                int rank = sq >> 3;
                int index = (col == Color::White) ? sq : (sq ^ 56); // Flip for black
                
                int mg = PawnValue + mg_pawnPST[index];
                int eg = PawnValue + eg_pawnPST[index];
                
                // Passed Pawns
                if (isPassedPawn(sq, col, opp_pawns)) {
                    int r = (col == Color::White) ? rank : 7 - rank;
                    mg += PASSED_PAWN_MG[r];
                    eg += PASSED_PAWN_EG[r];
                }

                // Doubled Pawns
                int pawns_on_file = __builtin_popcountll(my_pawns & FILE_MASKS[file]);
                if (pawns_on_file > 1) {
                    mg += DOUBLED_PAWN_MG;
                    eg += DOUBLED_PAWN_EG;
                }

                // Isolated Pawns
                uint64_t adj_files = 0;
                if (file > 0) adj_files |= FILE_MASKS[file - 1];
                if (file < 7) adj_files |= FILE_MASKS[file + 1];
                if ((my_pawns & adj_files) == 0) {
                    mg += ISOLATED_PAWN_MG;
                    eg += ISOLATED_PAWN_EG;
                }

                // Phalanx Pawns
                if (file < 7 && (my_pawns & (1ULL << (sq + 1)))) {
                    mg += PHALANX_PAWN_MG;
                    eg += PHALANX_PAWN_EG;
                }

                mgScore += mg * sign;
                egScore += eg * sign;
                bb &= (bb - 1);
            }

            // 2. KNIGHTS
            bb = board.pieces[c][KNIGHT];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                int index = (col == Color::White) ? sq : (sq ^ 56);
                mgScore += (KnightValue + mg_knightPST[index]) * sign;
                egScore += (KnightValue + eg_knightPST[index]) * sign;
                bb &= (bb - 1);
            }

            // 3. BISHOPS
            bb = board.pieces[c][BISHOP];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                int index = (col == Color::White) ? sq : (sq ^ 56);
                int mg = BishopValue + mg_bishopPST[index];
                int eg = BishopValue + eg_bishopPST[index];
                
                // Magic Bitboard Mobility
                int mob = __builtin_popcountll(Magics::get_bishop_attacks(static_cast<Square>(sq), occ));
                mg += mob * BISHOP_MOBILITY_MG;
                eg += mob * BISHOP_MOBILITY_EG;

                mgScore += mg * sign;
                egScore += eg * sign;
                bb &= (bb - 1);
            }

            // 4. ROOKS
            bb = board.pieces[c][ROOK];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                int index = (col == Color::White) ? sq : (sq ^ 56);
                int file = sq & 7;
                int mg = RookValue + mg_rookPST[index];
                int eg = RookValue + eg_rookPST[index];
                
                // Magic Bitboard Mobility
                int mob = __builtin_popcountll(Magics::get_rook_attacks(static_cast<Square>(sq), occ));
                mg += mob * ROOK_MOBILITY_MG;
                eg += mob * ROOK_MOBILITY_EG;

                // Open & Semi-Open Files
                if ((my_pawns & FILE_MASKS[file]) == 0) {
                    if ((opp_pawns & FILE_MASKS[file]) == 0) {
                        mg += ROOK_OPEN_FILE_MG; eg += ROOK_OPEN_FILE_EG;
                    } else {
                        mg += ROOK_SEMI_OPEN_MG; eg += ROOK_SEMI_OPEN_EG;
                    }
                }

                mgScore += mg * sign;
                egScore += eg * sign;
                bb &= (bb - 1);
            }

            // 5. QUEENS
            bb = board.pieces[c][QUEEN];
            while (bb) {
                int sq = __builtin_ctzll(bb);
                int index = (col == Color::White) ? sq : (sq ^ 56);
                int mg = QueenValue + mg_queenPST[index];
                int eg = QueenValue + eg_queenPST[index];
                
                // Utilizing your pre-existing get_queen_attacks function!
                int mob = __builtin_popcountll(Magics::get_queen_attacks(static_cast<Square>(sq), occ));
                mg += mob * QUEEN_MOBILITY_MG;
                eg += mob * QUEEN_MOBILITY_EG;

                mgScore += mg * sign;
                egScore += eg * sign;
                bb &= (bb - 1);
            }

            // 6. KINGS & KING SAFETY
            bb = board.pieces[c][KING];
            if (bb) {
                int sq = __builtin_ctzll(bb);
                int index = (col == Color::White) ? sq : (sq ^ 56);
                
                int mg = KingValue + mg_kingPST[index];
                int eg = KingValue + eg_kingPST[index];

                mg += evaluateKingSafety(sq, col, my_pawns, opp_pawns);

                mgScore += mg * sign;
                egScore += eg * sign;
            }
        }

        // --- TAPERED EVAL BLEND ---
        int finalScore = (mgScore * gamePhase + egScore * (24 - gamePhase)) / 24;

        // Always return the score relative to the side whose turn it is
        return (board.side_to_move == Color::White) ? finalScore : -finalScore;
    }

} // namespace Eval
