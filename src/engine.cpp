#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <sstream>

#include "types.hpp"
#include "zobrist.hpp" 
#include "board.hpp"
#include "magics.hpp"
#include "movegen.hpp"
#include "nnue.hpp"
#include "tt.hpp"
#include "search.hpp" 

// Instantiate Global Variables
NNUEWeights weights;
Accumulator accumulators[MAX_PLY]; 

U64 KnightAttacks[64];
U64 KingAttacks[64];

// Helper to load NNUE file
void load_nnue(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "Error: Could not find " << filename << "\n";
        std::cout << "Did you compile and run 'export_net.cpp' first to generate it?\n";
        exit(1);
    }
    file.read(reinterpret_cast<char*>(&weights), sizeof(weights));
}

// Helper to convert 0-63 to "e2", "e4", etc.
std::string sq_to_string(int sq) {
    char file = 'a' + (sq % 8);
    char rank = '1' + (sq / 8);
    return std::string{file, rank};
}

// Helper to convert "e2e4" into your engine's Move struct AND grab the correct pieces!
Move parse_move(const std::string& move_str, const Board& board) {
    Move m;
    m.from = (move_str[1] - '1') * 8 + (move_str[0] - 'a');
    m.to   = (move_str[3] - '1') * 8 + (move_str[2] - 'a');
    
    // CRITICAL FIX: We must identify the actual piece so NNUE accumulator updates don't crash
    m.piece = EMPTY;
    m.captured = EMPTY;
    m.promoted = EMPTY;

    // Scan bitboards to find what piece is moving and if anything is captured
    for (int p = 0; p < 12; ++p) {
        if (board.pieces[p] & (1ULL << m.from)) m.piece = p;
        if (board.pieces[p] & (1ULL << m.to))   m.captured = p;
    }

    // Handle En Passant captures (Pawn moved diagonally but target square was empty)
    if ((m.piece == 0 || m.piece == 6) && m.captured == EMPTY && (m.from % 8 != m.to % 8)) {
        m.captured = (m.piece == 0) ? 6 : 0; // Black pawn (6) or White pawn (0)
    }

    // Handle Promotion (e.g., e7e8q)
    if (move_str.length() == 5) {
        char p = move_str[4];
        bool is_white = (m.piece < 6); 
        if (p == 'q') m.promoted = is_white ? 4 : 10; // Queen
        if (p == 'r') m.promoted = is_white ? 3 : 9;  // Rook
        if (p == 'b') m.promoted = is_white ? 2 : 8;  // Bishop
        if (p == 'n') m.promoted = is_white ? 1 : 7;  // Knight
    }

    return m;
}

int main() {
    // 1. Initialize Zobrist & pre-calculated move tables!
    Zobrist::init(); 
    init_magics();
    
    // 2. Initialize Memory & Neural Net
    init_tt();                 
    load_nnue("chessboi.bin"); 

    Board board;
    
    // Print that the engine started successfully
    std::cout << "ChessboiV4 AI - UCI Ready\n";

    // 3. The UCI Protocol Loop
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "uci") {
            std::cout << "id name ChessboiV4\n";
            std::cout << "id author Hugo\n";
            std::cout << "uciok\n";
        } 
        else if (command == "isready") {
            std::cout << "readyok\n";
        } 
        else if (command == "ucinewgame") {
            init_tt(); // Clear the transposition table for a fresh game
        } 
        else if (command == "position") {
            std::string token;
            iss >> token;
            if (token == "startpos") {
                board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                iss >> token; // read "moves" if it exists
            } else if (token == "fen") {
                std::string fen = "";
                for (int i = 0; i < 6; i++) {
                    std::string fen_token;
                    iss >> fen_token;
                    fen += fen_token + " ";
                }
                board.load_fen(fen);
                iss >> token; // read "moves" if it exists
            }
            
            // Apply all moves played in the game so far
            if (token == "moves") {
                std::string move_str;
                while (iss >> move_str) {
                    Move m = parse_move(move_str, board); // Pass the board!
                    board.make_move(m); 
                }
            }

            // CRITICAL FIX: Refresh accumulator at ply 0 AFTER setting up the entire board state!
            refresh_accumulator(board, 0);
        } 
        else if (command == "go") {
            int search_time = 3000; // Default fallback: 3 seconds
            int wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0;
            std::string token;
            
            // Parse Lichess time controls dynamically
            while (iss >> token) {
                if (token == "wtime") iss >> wtime;
                else if (token == "btime") iss >> btime;
                else if (token == "winc") iss >> winc;
                else if (token == "binc") iss >> binc;
                else if (token == "movetime") iss >> movetime;
            }
            
            if (movetime > 0) {
                search_time = movetime;
            } else if (wtime > 0 || btime > 0) {
                // Determine our current clock time based on whose turn it is
                int my_time = (board.side_to_move == WHITE) ? wtime : btime;
                int my_inc  = (board.side_to_move == WHITE) ? winc : binc;
                
                // Formula: Spend 1/30th of remaining time + a chunk of increment
                search_time = (my_time / 30) + (my_inc * 3 / 4);
                
                // Safety net: Avoid flagging if time gets very low!
                if (search_time >= my_time) search_time = my_time - 500;
                if (search_time < 100) search_time = 100; // Minimum 0.1s
                if (my_time < 200) search_time = 10; // Extreme time pressure emergency
            }
            
            // Fire up the Iterative Deepening Search!
            Move best = Search::search_best_move(board, search_time);
            
            // Output the required UCI response
            if (best.from != best.to) { // Check valid move
                std::string uci_move = sq_to_string(best.from) + sq_to_string(best.to);
                // Attach promotion char if necessary
                if (best.promoted == 4 || best.promoted == 10) uci_move += 'q';
                else if (best.promoted == 3 || best.promoted == 9) uci_move += 'r';
                else if (best.promoted == 2 || best.promoted == 8) uci_move += 'b';
                else if (best.promoted == 1 || best.promoted == 7) uci_move += 'n';
                
                std::cout << "bestmove " << uci_move << "\n";
            } else {
                std::cout << "bestmove 0000\n"; // Fallback if checkmated
            }
        } 
        else if (command == "quit") {
            break;
        }
    }

    return 0;
}
