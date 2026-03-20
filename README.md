# HomeboiChessbot (v3.1 - The Syzygy Update)
**HomeboiChessbot** (aka Chessboi) is a fully functional, UCI-compatible chess engine written from scratch in C++. 

Version 3.1 builds upon the massive 64-bit Bitboard rewrite by introducing perfect endgame play via **Syzygy Tablebases**. Chessboi's node-per-second (NPS) speed, tactical vision, and endgame technique have skyrocketed to highly competitive levels.

## What's New in v3.1
* **Syzygy Tablebase Support:** Chessboi now plays 3, 4, and 5-piece endgames perfectly! By natively probing Syzygy tablebases, the engine instantly knows the objective outcome (Win/Draw/Loss) and Distance to Zero (DTZ) of endgame positions, bypassing the search tree entirely for flawless conversion of winning endgames.
* **All-in-One Release:** The necessary Syzygy tablebase files and the Polyglot opening book (`book.bin`) are now conveniently bundled and available for download directly in the **v3.1 Release**! *(Note: The Syzygy tablebase files were originally obtained from the standard Lichess database at [https://tablebase.lichess.ovh/tables/standard/](https://tablebase.lichess.ovh/tables/standard/)).*

## Core Architecture (Introduced in v3.0)
* **Complete Core Rewrite (Hybrid Architecture):** Moved away from a slow, pure 1D array to a blazing-fast **64-bit Bitboard Architecture**. However, Chessboi utilizes a *Hybrid approach*, maintaining a lightweight `piece_on[64]` array alongside the bitboards. This allows for **O(1) branchless piece lookups** during move making, unmaking, and scoring!
* **16-Bit Move Encoding:** Moves are now packed into a single 16-bit integer using bitwise operations, dropping the memory footprint of a move list from 4096 bytes down to just 516 bytes.
* **Magic Bitboards:** Sliding piece (Rook, Bishop, Queen) attack generation now uses pre-calculated Magic Bitboards for instant O(1) attack lookups.
* **Polyglot Opening Book:** Chessboi natively reads standard `.bin` Polyglot opening books, loading them directly into RAM for instant (`< 0.1ms`) binary-search lookups. He will now play grandmaster opening theory without using the clock!
* **Zobrist Hashing & Transposition Table:** Implemented a massive memory vault that stores millions of previously evaluated positions, allowing the engine to instantly recall scores instead of recalculating identical board states.
* **Advanced Search Heuristics:** Implemented Null Move Pruning (NMP) and Late Move Reductions (LMR) to drastically trim useless branches from the search tree.
* **Blitz & Bullet Capable:** Dynamic time management (Soft/Hard limits) allows the engine to pace itself, allocate extra time for complex tactical sequences, and play fast time controls without flagging.

---

## Engine Features

### Protocol & Interface
* Fully supports the **Universal Chess Interface (UCI)**, allowing it to be plugged into almost any modern chess GUI (En Croissant, CuteChess, Arena, etc.).

### Search Algorithm (The Brain)
* **Principal Variation Search (PVS / NegaScout):** A highly optimized zero-window search algorithm that drastically reduces the search tree, scaling tactical depth much higher than standard Alpha-Beta.
* **Iterative Deepening:** Searches progressively deeper, allowing for dynamic time management and move ordering.
* **Quiescence Search (With Check Evasions):** Continues searching capture chains and forcing checks at the end of the main search to completely eliminate the "Horizon Effect" and tactical blindspots.
* **Null Move Pruning (NMP):** Gives the opponent a "free move" to prove a position is overwhelmingly winning, drastically reducing the search space.
* **Late Move Reductions (LMR):** Skims "boring" or quiet moves at shallower depths to focus computational power on critical tactical lines.

### Move Ordering (The Intuition)
* **TT / Hash Moves:** Instantly searches the best move found in previous iterations.
* **O(1) MVV-LVA:** (Most Valuable Victim - Least Valuable Attacker) efficiently searches the best captures first without requiring loops.
* **Killer Moves & History Heuristic:** Remembers quiet moves that caused cutoffs in sibling nodes to prioritize them later.

### Advanced Positional Evaluation (The Soul)
* **Tapered Evaluation:** Dynamically blends Midgame and Endgame piece-square tables based on the exact phase of the game (e.g., Centralizing the King in the endgame).
* **Pawn Structures:** Actively penalizes doubled and isolated pawns to prevent long-term positional weaknesses.
* **Piece Synergies:** Recognizes and rewards the Bishop Pair, and places Rooks on Open and Semi-Open files.
* **Passed Pawns:** Recognizes and pushes unstoppable pawns, scaling the reward the closer they get to promotion.
* **King Safety:** Evaluates pawn shields and open files near the King to avoid getting mated in the middlegame.

---

## How to Play Against HomeboiChessbot
Because Chessboi uses the UCI protocol, it does not have its own graphical interface. You need to plug it into a Chess GUI.

**Recommended GUIs:**
* [En Croissant](https://encroissant.org/) (Tested on)
* [CuteChess](https://cutechess.com/) 
* [Arena Chess GUI](http://www.playwitharena.de/) (Tested on)

**Setup Instructions:**
1. Download the compiled executable from the **v3.1 Releases** tab (or build it yourself).
2. Download the Polyglot `book.bin` and the bundled **Syzygy Tablebase files**, both available directly in the v3.1 Release assets.
3. Place `book.bin` in the exact same folder as the Chessboi executable.
4. Extract the Syzygy files into a folder on your computer.
5. Open your Chess GUI and go to **Engine Settings** -> **Install New Engine**. Select the `Chessboi` executable file.
6. **Enable Syzygy:** In the engine's UCI parameters inside your GUI, find the `SyzygyPath` option and point it to the folder where you extracted the Syzygy files.
7. Start a new game and select Chessboi as your opponent!

---

## 🛠️ Building from Source
If you want to compile Chessboi yourself, clone the repository and compile using `g++`. *(Make sure to use the `-O3` flag for maximum speed and `-std=c++20`!)*

**Linux / macOS:**
```bash
git clone https://github.com/schenegghugo/HomeboiChessbot.git
cd HomeboiChessbot/src
g++ -O3 -std=c++20 -Wall -Wextra main.cpp -o Chessboi
./Chessboi
```

**Windows (MinGW):**
```bash
g++ -O3 -std=c++20 -Wall -Wextra main.cpp -o Chessboi.exe
Chessboi.exe
```

---

## 🗺️ Roadmap / Future Updates
Chessboi has officially crossed the threshold into a highly competitive search engine. The next major milestones focus on evaluating positions with cutting-edge ML and hardware optimization:
- [x] **Bitboard Architecture:** Blazing fast 64-bit board representation and Magic Bitboards.
- [x] **Transposition Tables:** Zobrist Hashing for position recall.
- [x] **Quiescence Search:** Defeating the Horizon Effect.
- [x] **Principal Variation Search (PVS):** A zero-window search algorithm.
- [x] **Advanced Positional Evaluation:** Penalizing doubled/isolated pawns and rewarding piece mobility.
- [x] **Opening Book Support:** Natively reading `.bin` Polyglot opening books to survive aggressive theory.
- [x] **Endgame Tablebases:** Syzygy tablebase support for perfect play in 3, 4, and 5-piece endgames.
- [ ] **NNUE Evaluation:** Upgrading from Hand-Crafted Evaluation (HCE) to a strictly Neural Network-based evaluation for a massive Elo leap.
- [ ] **Multi-threading (Lazy SMP):** Allowing the engine to utilize multiple CPU cores simultaneously.

## Author
Created by **Hugo Schenegg**. 
Feel free to open an issue or submit a pull request if you find a bug or want to suggest a feature!
