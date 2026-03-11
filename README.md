# HomeboiChessbot
**HomeboiChessbot** is a fully functional, UCI-compatible chess engine written from scratch in C++. 

## Features (v1.0)
* **Protocol:** Fully supports the Universal Chess Interface (UCI), allowing it to be plugged into almost any modern chess GUI.
* **Board Representation:** 1D 64-Square Array.
* **Move Generation:** Blazing fast pseudo-legal move generation within the search tree, with strict legal-move filtering at the root to prevent illegal timelines.
* **Search Algorithm:** Negamax with Alpha-Beta Pruning.
* **Move Ordering:** MVV-LVA (Most Valuable Victim - Least Valuable Attacker) to efficiently prune bad branches and search captures first.
* **Evaluation:** 
  * Material counting.
  * Piece-Square Tables (PSTs) to encourage central control, rapid development, and early castling.
  * Tuned for an aggressive, forward-moving playstyle.

## How to Play Against HomeboiChessbot
Because TalBot uses the UCI protocol, it does not have its own graphical interface. You need to plug it into a Chess GUI.

**Recommended GUIs:**
* [En Croissant](https://encroissant.org/) (Highly recommended)
* [Arena Chess GUI](http://www.playwitharena.de/)
* [CuteChess](https://cutechess.com/)

**Setup Instructions:**
1. Download the compiled executable from the Releases tab (or build it yourself).
2. Open your Chess GUI.
3. Go to **Engine Settings** -> **Install New Engine**.
4. Select the `chessbot` executable file.
5. Start a new game and select TalBot as your opponent!

*(Note: In v1.0, HomeboiChessbot searches at a fixed depth of 5. Playing at very fast Blitz/Bullet time controls may cause it to flag on time. Iterative deepening and time management are coming in the next update!)*

## Building from Source
If you want to compile TalBot yourself, clone the repository and compile using `g++` (Make sure to use the `-O3` flag for maximum speed!).

**Linux / macOS:**
```bash
git clone https://github.com/schenegghugo/HomeboiChessbot/
cd talbot/src
g++ main.cpp precomputed.cpp -O3 -o chessbot
./talbot
```

**Windows (MinGW):**
```bash
g++ main.cpp precomputed.cpp -O3 -o chessbot.exe
talbot.exe
```

## Roadmap / Future Updates
TalBot is continuously improving. The next major milestones are:
- [ ] **Time Management:** Implementing Iterative Deepening so HomeboiChessbot can manage its own clock and play Blitz/Bullet matches.
- [ ] **Transposition Tables:** Adding a Hash Map to remember previously calculated positions and double the search speed.
- [ ] **Tapered Evaluation:** Smoothly transitioning the Piece-Square Tables from the middlegame to the endgame (e.g., encouraging the King to centralize when the Queens are off the board).
- [ ] **Quiescence Search:** Extending the search depth for volatile positions (like capture chains) to prevent the Horizon Effect.

## Author
Created by **Hugo Schenegg**. 
Feel free to open an issue or submit a pull request if you find a bug or want to suggest a feature!
```
