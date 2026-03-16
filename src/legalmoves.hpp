#ifndef LEGALMOVES_HPP
#define LEGALMOVES_HPP

#include <vector>
#include "board.hpp"
// Include your move structures, make_move, and movegen headers:
// #include "move.hpp"
// #include "movegen.hpp"
// #include "makemove.hpp" 

inline std::vector<Move> get_legal_moves(const Board& board) {
    std::vector<Move> pseudo_moves;
    // Assume your generator fills this vector
    movegen::generate_moves(board, pseudo_moves); 
    
    std::vector<Move> legal_moves;
    
    Color friendly_color = board.side_to_move;
    Color enemy_color = (friendly_color == Color::White) ? Color::Black : Color::White;

    for (const Move& move : pseudo_moves) {
        // 1. Jump forward in time by simply making a copy (No Undo logic required!)
        Board copy = board;
        
        // 2. Play the move on the copy
        // Depending on your setup, this might be copy.make_move(move) or make_move(copy, move)
        make_move(copy, move); 
        
        // 3. Find where our King is sitting NOW
        Bitboard king_bb = copy.pieces[static_cast<int>(friendly_color)][KING];
        
        if (king_bb == 0) continue; // Failsafe in case King was somehow captured
        
        // __builtin_ctzll finds the exact square index from the bitboard instantly!
        int king_square = __builtin_ctzll(king_bb);

        // 4. If the King is NOT attacked, the move is perfectly legal!
        if (!copy.is_square_attacked(king_square, enemy_color)) {
            legal_moves.push_back(move);
        }
    }
    
    return legal_moves;
}

#endif
