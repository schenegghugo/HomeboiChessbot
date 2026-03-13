#include "precomputed.h"
#include "uci.h"
#include "book.h" 

int main() {
    // 1. Initialize engine mathematics
    precomputeData();

    // 2. Load the Polyglot Opening Book into RAM!
    loadOpeningBook("/home/hugo/Work/projects/Chessbot/src/Cerebellum_Light_Poly.bin");

    // 3. Start listening for GUI commands
    uciLoop();

    return 0;
}
