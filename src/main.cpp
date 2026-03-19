#include "board.hpp"
#include "movegen.hpp"
#include "magics.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "book.hpp" // <-- Add this
#include <iostream>
#include "tt.hpp"

int main() {
    // Initialize move generation lookup tables
    Attacks::init_leapers();
    Magics::init_magic_bitboards();

    // Transition Table, remembers the best moves
    Zobrist::init();
    TT::init();

    // Load Opening Book into RAM
    Book::load("book.bin"); // <-- Opening theory

    // Start listening to the GUI!
    UCI::uci_loop();

    return 0;
}
