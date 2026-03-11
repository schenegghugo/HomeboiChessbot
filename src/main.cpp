#include "precomputed.h"
#include "uci.h"

int main() {
    // 1. Initialize engine mathematics
    precomputeData();

    // 2. Start listening for GUI commands
    uciLoop();

    return 0;
}
