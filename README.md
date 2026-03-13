# HomeboiChessbot
**HomeboiChessbot** (aka Chessboi) is a fully functional, UCI-compatible chess engine written from scratch in C++. 

## Features (v2.2 - The Positional Update)
* **Protocol:** Fully supports the Universal Chess Interface (UCI), allowing it to be plugged into almost any modern chess GUI.
* **Board Representation:** 1D 64-Square Array.
* **Move Generation:** Blazing fast pseudo-legal move generation within the search tree, with strict legal-move filtering at the root to prevent illegal timelines.
* **Search Algorithm (The Brain):** 
  * **Principal Variation Search (PVS / NegaScout):** A highly optimized zero-window search algorithm that drastically reduces the search tree, scaling tactical depth much higher than standard Alpha-Beta.
  * **Iterative Deepening:** Searches progressively deeper, allowing for dynamic time management.
  * **Quiescence Search (With Check Evasions):** Continues searching capture chains and forcing checks at the end of the main search to completely eliminate the "Horizon Effect" and tactical blindspots.
  * **Null Move Pruning:** Gives the opponent a "free move" to prove a position is overwhelmingly winning, drastically reducing search space.
  * **Late Move Reductions (LMR):** Skims "boring" moves at shallower depths to focus computational power on critical tactical lines.
* **Move Ordering (The Engine's Intuition):** 
  * Hash Moves (from the Transposition Table).
  * MVV-LVA (Most Valuable Victim - Least Valuable Attacker) to efficiently search the best captures first.
  * **Killer Moves:** Remembers quiet moves that caused cutoffs in sibling nodes to prioritize them later.
* **Transposition Table (Zobrist Hashing):** A massive memory vault that stores millions of previously evaluated positions, allowing Chessboi to instantly recall scores instead of recalculating identical board states.
* **Advanced Positional Evaluation (The Soul):** 
  * **Tapered Evaluation:** Dynamically blends Midgame and Endgame piece-square tables based on the exact phase of the game (e.g., Centralizing the King and Queen in the endgame).
  * **Pawn Structures:** Actively penalizes doubled and isolated pawns to prevent long-term positional weaknesses.
  * **Piece Synergies:** Recognizes and rewards the Bishop Pair, and places Rooks on Open and Semi-Open files.
  * **Passed Pawns:** Recognizes and pushes unstoppable pawns, scaling the reward the closer they get to promotion.
  * **King Safety:** Evaluates pawn shields and open files near the King to avoid getting mated in the middlegame.
* **Smart Time Management:** Uses Soft Limits, Hard Limits, and Complexity Scaling to spend extra time on volatile tactical positions and move instantly when forced.

## How to Play Against HomeboiChessbot
Because Chessboi uses the UCI protocol, it does not have its own graphical interface. You need to plug it into a Chess GUI.

**Recommended GUIs:**
* [En Croissant](https://encroissant.org/) (Tested on)
* [Arena Chess GUI](http://www.playwitharena.de/)
* [CuteChess](https://cutechess.com/) (Tested on)

**Setup Instructions:**
1. Download the compiled executable from the Releases tab (or build it yourself).
2. Open your Chess GUI.
3. Go to **Engine Settings** -> **Install New Engine**.
4. Select the `chessboi` executable file.
5. Start a new game and select Chessboi as your opponent!

*(Note: As of v2.0, Chessboi fully supports Blitz and Bullet time controls! He will manage his own clock dynamically to avoid flagging.)*

## Building from Source
If you want to compile Chessboi yourself, clone the repository and compile using `g++` (Make sure to use the `-O3` flag for maximum speed!).

**Linux / macOS:**
```bash
git clone https://github.com/schenegghugo/HomeboiChessbot/
cd HomeboiChessbot/src
g++ *.cpp -std=c++17 -O3 -o chessboi
./chessboi
```

**Windows (MinGW):**
```bash
g++ *.cpp -std=c++17 -O3 -o chessboi.exe
chessboi.exe
```

## Roadmap / Future Updates
Chessboi has crossed the threshold into a competitive search engine. The next major milestones focus on advanced positional understanding and search optimization:
- [x] **Time Management:** Iterative Deepening and dynamic clock scaling.
- [x] **Transposition Tables:** Zobrist Hashing for position recall.
- [x] **Quiescence Search:** Defeating the Horizon Effect.
- [x] **Principal Variation Search (PVS):** A zero-window search algorithm to squeeze even more tactical depth out of the engine and eliminate 1000-centipawn blunders.
- [x] **Check Evasions in Q-Search:** Expanding Quiescence Search to handle forcing checks, not just captures.
- [x] **Advanced Positional Evaluation:** Penalizing doubled/isolated pawns and rewarding piece mobility to prevent getting squeezed in the middlegame.
- [ ] **Opening Book Support:** Allowing Chessboi to read `.bin` Polyglot opening books to survive aggressive theory.
- [ ] **Endgame Tablebases:** Syzygy tablebase support for perfect play in 5 and 6-piece endgames.

## Author
Created by **Hugo Schenegg**. 
Feel free to open an issue or submit a pull request if you find a bug or want to suggest a feature!

