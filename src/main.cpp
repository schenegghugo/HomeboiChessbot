#include "board.hpp"
#include "movegen.hpp"
#include "magics.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "book.hpp" 
#include "tt.hpp"
#include "tbprobe.h"  // <--- ADD FATHOM HEADER
#include <iostream>

int main() {
    // Initialize move generation lookup tables
    Attacks::init_leapers();
    Magics::init_magic_bitboards();

    // Transition Table, remembers the best moves
    Zobrist::init(); 
    TT::init();

    // Load Opening Book into RAM
    Book::load("book.bin"); 

    // Initialize Syzygy Tablebases using your exact path
    bool tb_loaded = tb_init("/home/hugo/Work/projects/Chess/ChessboiV3/syzygy"); 
    
    if (tb_loaded || TB_LARGEST > 0) {
        std::cout << "info string Syzygy loaded! Max pieces: " << TB_LARGEST << std::endl;
    } else {
        std::cout << "info string FAILED to load Syzygy. Check if .rtbw/.rtbz files are in the folder!" << std::endl;
    }

    // Start listening to the GUI!
    UCI::uci_loop();

    return 0;
}
