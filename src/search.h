#ifndef SEARCH_H
#define SEARCH_H

#include "legalmoves.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstdint>
#include "board.h"
#include "movegen.h"
#include "makemove.h"
#include "pem.h"

// --- TIME MANAGEMENT GLOBALS ---
inline long long startTime = 0;
inline long long allocatedTime = 0;
inline bool timeIsUp = false;
inline int nodesSearched = 0;

inline long long getTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// --- THE TRANSPOSITION TABLE (THE VAULT) ---
const int hashfEXACT = 0; 
const int hashfALPHA = 1; 
const int hashfBETA  = 2; 

struct TTEntry {
    uint64_t hashKey;
    int depth;
    int flag;
    int score;
    int bestMoveFrom;
    int bestMoveTo;
};

const int TT_SIZE = 4194304; 
const int TT_MASK = TT_SIZE - 1;
inline std::vector<TTEntry> TT(TT_SIZE);

// --- KILLER MOVES ---
const int MAX_PLY = 64;
inline Move killerMoves[MAX_PLY][2];

// --- MOVE ORDERING ---
inline int scoreMove(const BoardState& board, Move move, int ttFrom, int ttTo, int plyFromRoot) {
    if (move.fromSquare == ttFrom && move.toSquare == ttTo) return 10000000; 

    int movingPiece = board.squares[move.fromSquare];
    int targetPiece = board.squares[move.toSquare];

    if (targetPiece != EMPTY) {
        int attackerValue = getPieceValue(movingPiece) / 100;
        int victimValue = getPieceValue(targetPiece) / 100;
        return 1000000 + (10 * victimValue) - attackerValue; 
    }
    
    if (plyFromRoot < MAX_PLY) {
        if (killerMoves[plyFromRoot][0].fromSquare == move.fromSquare && killerMoves[plyFromRoot][0].toSquare == move.toSquare) return 900000;
        if (killerMoves[plyFromRoot][1].fromSquare == move.fromSquare && killerMoves[plyFromRoot][1].toSquare == move.toSquare) return 800000;
    }

    return 0; 
}

// --- QUIESCENCE SEARCH (WITH CHECK EVASIONS) ---
inline int quiescence(BoardState& board, int alpha, int beta, int plyFromRoot) {
    if ((nodesSearched++ & 2047) == 0) {
        if (getTimeMs() - startTime > allocatedTime) timeIsUp = true;
    }
    if (timeIsUp) return 0;

    if (plyFromRoot >= MAX_PLY - 1) return evaluate(board);

    bool isInCheck = inCheck(board, board.sideToMove);
    int standPat = -1000000;

    if (!isInCheck) {
        standPat = evaluate(board);
        if (standPat >= beta) return beta;
        if (alpha < standPat) alpha = standPat;
    }

    std::vector<Move> moves = generateMoves(board);
    std::vector<Move> movesToSearch;

    if (isInCheck) {
        movesToSearch = moves;
    } else {
        for (Move m : moves) {
            if (board.squares[m.toSquare] != EMPTY) {
                movesToSearch.push_back(m);
            }
        }
    }

    std::sort(movesToSearch.begin(), movesToSearch.end(), [&board, plyFromRoot](Move a, Move b) {
        return scoreMove(board, a, -1, -1, plyFromRoot) > scoreMove(board, b, -1, -1, plyFromRoot);
    });

    int bestScore = isInCheck ? -1000000 : standPat;

    for (Move move : movesToSearch) {
        int targetPiece = board.squares[move.toSquare];
        if (targetPiece == W_KING || targetPiece == B_KING) return 100000;

        UndoData undo = makeMove(board, move);
        int score = -quiescence(board, -beta, -alpha, plyFromRoot + 1);
        undoMove(board, move, undo);

        if (timeIsUp) return 0;

        if (score > bestScore) bestScore = score;
        if (score >= beta) return beta; 
        if (score > alpha) alpha = score;
    }

    if (isInCheck && bestScore <= -90000) {
        return -99999 + plyFromRoot; 
    }

    return alpha;
}

// --- NEGAMAX WITH PVS, NULL MOVE, KILLERS & LMR ---
inline int negamax(BoardState& board, int depth, int alpha, int beta, int plyFromRoot, bool allowNull = true) {
    int alphaOrig = alpha; 

    if ((nodesSearched++ & 2047) == 0) {
        if (getTimeMs() - startTime > allocatedTime) timeIsUp = true;
    }
    if (timeIsUp) return 0;

    if (board.ply > 0 && isRepetition(board)) return 0; 

    int ttFrom = -1, ttTo = -1;
    TTEntry& ttEntry = TT[board.hashKey & TT_MASK]; 

    if (ttEntry.hashKey == board.hashKey) {
        ttFrom = ttEntry.bestMoveFrom;
        ttTo = ttEntry.bestMoveTo;
        if (ttEntry.depth >= depth) {
            if (ttEntry.flag == hashfEXACT) return ttEntry.score;
            if (ttEntry.flag == hashfALPHA && ttEntry.score <= alpha) return alpha;
            if (ttEntry.flag == hashfBETA && ttEntry.score >= beta) return beta;
        }
    }

    if (depth <= 0) return quiescence(board, alpha, beta, plyFromRoot);

    bool currentlyInCheck = inCheck(board, board.sideToMove);
    
    // Null Move Pruning
    if (allowNull && depth >= 3 && !currentlyInCheck) {
        int staticEval = evaluate(board);
        if (staticEval >= beta) {
            BoardState nullBoard = board;
            nullBoard.sideToMove = (board.sideToMove == WHITE) ? BLACK : WHITE;
            nullBoard.enPassantSquare = -1; 
            
            int R = 2; 
            int nullScore = -negamax(nullBoard, depth - 1 - R, -beta, -beta + 1, plyFromRoot + 1, false);
            if (nullScore >= beta) return beta; 
        }
    }

    std::vector<Move> moves = generateMoves(board);
    if (moves.empty()) return -99999 + plyFromRoot; 

    std::sort(moves.begin(), moves.end(), [&board, ttFrom, ttTo, plyFromRoot](Move a, Move b) {
        return scoreMove(board, a, ttFrom, ttTo, plyFromRoot) > scoreMove(board, b, ttFrom, ttTo, plyFromRoot);
    });

    int bestScore = -1000000;
    Move bestMoveThisNode = moves[0];
    int movesSearched = 0; 

    for (Move move : moves) {
        int targetPiece = board.squares[move.toSquare];
        if (targetPiece == W_KING || targetPiece == B_KING) return 100000; 

        UndoData undo = makeMove(board, move);
        
        bool givesCheck = inCheck(board, board.sideToMove);
        int extension = (givesCheck && depth < 20) ? 1 : 0;
        bool isQuiet = (targetPiece == EMPTY) && !givesCheck && (move.promotedPiece == EMPTY);

        int currentScore;

        // --- PRINCIPAL VARIATION SEARCH (PVS) ---
        if (movesSearched == 0) {
            currentScore = -negamax(board, depth - 1 + extension, -beta, -alpha, plyFromRoot + 1, true);
        } else {
            if (movesSearched >= 3 && depth >= 3 && isQuiet) {
                int reduction = (movesSearched > 6) ? 2 : 1; 
                currentScore = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, plyFromRoot + 1, true);
                
                if (currentScore > alpha) {
                    currentScore = -negamax(board, depth - 1 + extension, -alpha - 1, -alpha, plyFromRoot + 1, true);
                }
            } else {
                currentScore = -negamax(board, depth - 1 + extension, -alpha - 1, -alpha, plyFromRoot + 1, true);
            }

            if (currentScore > alpha && currentScore < beta) {
                currentScore = -negamax(board, depth - 1 + extension, -beta, -alpha, plyFromRoot + 1, true);
            }
        }
        
        undoMove(board, move, undo);
        movesSearched++;

        if (timeIsUp) return 0;

        if (currentScore > bestScore) {
            bestScore = currentScore;
            bestMoveThisNode = move;
        }
        if (bestScore > alpha) alpha = bestScore;
        
        if (alpha >= beta) {
            if (board.squares[move.toSquare] == EMPTY && plyFromRoot < MAX_PLY) {
                if (move.fromSquare != killerMoves[plyFromRoot][0].fromSquare || move.toSquare != killerMoves[plyFromRoot][0].toSquare) {
                    killerMoves[plyFromRoot][1] = killerMoves[plyFromRoot][0];
                    killerMoves[plyFromRoot][0] = move;
                }
            }
            break; 
        }
    }

    if (!timeIsUp) {
        ttEntry.hashKey = board.hashKey;
        ttEntry.depth = depth;
        ttEntry.score = bestScore;
        ttEntry.bestMoveFrom = bestMoveThisNode.fromSquare;
        ttEntry.bestMoveTo = bestMoveThisNode.toSquare;

        if (bestScore <= alphaOrig) ttEntry.flag = hashfALPHA;
        else if (bestScore >= beta) ttEntry.flag = hashfBETA;
        else ttEntry.flag = hashfEXACT;
    }

    return bestScore;
}

// --- ITERATIVE DEEPENING & SMART TIME MANAGEMENT (PVS ROOT) ---
inline Move getBestMoveIterative(BoardState& board, long long baseTimeMs) {
    startTime = getTimeMs();
    
    std::vector<Move> moves = getLegalMoves(board); 
    if (moves.empty()) {
        std::cout << "bestmove 0000\n";
        return Move(0, 0, Flag_None, EMPTY); 
    }

    if (moves.size() == 1) {
        std::cout << "info string Only 1 legal move! Playing instantly.\n";
        return moves[0];
    }

    long long softTimeLimit = baseTimeMs;           
    long long hardTimeLimit = baseTimeMs * 3;       
    allocatedTime = hardTimeLimit;                  
    
    timeIsUp = false;
    nodesSearched = 0;

    for (int i = 0; i < MAX_PLY; i++) {
        killerMoves[i][0] = Move(0, 0, Flag_None, EMPTY);
        killerMoves[i][1] = Move(0, 0, Flag_None, EMPTY);
    }

    Move bestMoveOverall = moves[0];
    Move previousBestMove = moves[0];

    for (int depth = 1; depth <= MAX_PLY; depth++) {
        Move bestMoveThisDepth = moves[0];
        int bestScoreThisDepth = -1000000;
        
        int alpha = -1000000;
        int beta = 1000000;
        int movesSearched = 0;

        int ttFrom = -1, ttTo = -1;
        if (TT[board.hashKey & TT_MASK].hashKey == board.hashKey) {
            ttFrom = TT[board.hashKey & TT_MASK].bestMoveFrom;
            ttTo = TT[board.hashKey & TT_MASK].bestMoveTo;
        }

        std::sort(moves.begin(), moves.end(), [&board, ttFrom, ttTo](Move a, Move b) {
            return scoreMove(board, a, ttFrom, ttTo, 0) > scoreMove(board, b, ttFrom, ttTo, 0);
        });

        for (Move move : moves) {
            UndoData undo = makeMove(board, move);
            
            bool givesCheck = inCheck(board, board.sideToMove);
            int extension = givesCheck ? 1 : 0;
            int score;
            
            if (movesSearched == 0) {
                score = -negamax(board, depth - 1 + extension, -beta, -alpha, 1, true);
            } else {
                score = -negamax(board, depth - 1 + extension, -alpha - 1, -alpha, 1, true);
                if (score > alpha && score < beta) {
                    score = -negamax(board, depth - 1 + extension, -beta, -alpha, 1, true);
                }
            }

            undoMove(board, move, undo);
            movesSearched++;

            if (timeIsUp) break;

            if (score > bestScoreThisDepth) {
                bestScoreThisDepth = score;
                bestMoveThisDepth = move;
            }
            if (score > alpha) alpha = score;
        }

        if (timeIsUp) break; 

        bestMoveOverall = bestMoveThisDepth;
        long long timeSpent = getTimeMs() - startTime;

        std::cout << "info depth " << depth << " score cp " << bestScoreThisDepth 
                  << " time " << timeSpent << " nodes " << nodesSearched << "\n";
        
        if (depth > 1 && (bestMoveOverall.fromSquare != previousBestMove.fromSquare || bestMoveOverall.toSquare != previousBestMove.toSquare)) {
            softTimeLimit += baseTimeMs / 2; 
            if (softTimeLimit > hardTimeLimit) softTimeLimit = hardTimeLimit;
        }
        previousBestMove = bestMoveOverall;

        if (bestScoreThisDepth > 90000 || bestScoreThisDepth < -90000) break;

        if (timeSpent > softTimeLimit * 0.6) {
            break; 
        }
    }

    return bestMoveOverall;
}

#endif
