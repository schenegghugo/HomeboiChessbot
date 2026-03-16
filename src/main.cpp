#include "board.hpp"
#include "movegen.hpp"
#include "magics.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "book.hpp" // <-- Add this
#include <iostream>

int main() {
    // Initialize move generation lookup tables
    Attacks::init_leapers();
    Magics::init_magic_bitboards();

    // Transition Table, remembers the best moves
    TT::init_zobrist();
    TT::init_tt();

    // Load Opening Book into RAM
    Book::load("book.bin"); // <-- Add this

    // Start listening to the GUI!
    UCI::uci_loop();

    return 0;
}
