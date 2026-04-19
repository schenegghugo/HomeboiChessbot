import subprocess
import random
import chess

print("Starting Chessboi Datagen (Self-Play)...")

# Boot up your C++ UCI Engine in the background
engine = subprocess.Popen(
    ["./engine"], 
    stdin=subprocess.PIPE, 
    stdout=subprocess.PIPE, 
    text=True
)

def get_engine_move(fen):
    # Ask the C++ engine to evaluate the current board for 50 milliseconds
    engine.stdin.write(f"position fen {fen}\n")
    engine.stdin.write("go movetime 50\n")
    engine.stdin.flush()

    # Read the engine's output until it spits out 'bestmove'
    while True:
        line = engine.stdout.readline().strip()
        if line.startswith("bestmove"):
            return line.split()[1] # Returns the "e2e4" part

dataset = []
NUM_GAMES = 50 # Let's generate 50 games for our first real batch!

for game_id in range(NUM_GAMES):
    board = chess.Board()

    # 1. Play 4 random moves so the engine encounters unique positions every game
    for _ in range(4):
        random_move = random.choice(list(board.legal_moves))
        board.push(random_move)

    game_fens = []

    # 2. Engine vs Engine Deathmatch
    while not board.is_game_over(claim_draw=True):
        fen = board.fen()
        game_fens.append(fen)

        best_move_str = get_engine_move(fen)

        # Safety catch if engine gets checkmated or returns an error
        if best_move_str == "0000":
            break

        move = chess.Move.from_uci(best_move_str)
        if move in board.legal_moves:
            board.push(move)
        else:
            print(f"Engine hallucinated an illegal move: {best_move_str}")
            break

        # Stop infinite games where engines shuffle pieces aimlessly
        if len(board.move_stack) > 150:
            break

    # 3. Game Over! Figure out who won.
    result_str = board.result(claim_draw=True)
    score = 0.5 # Default to Draw
    if result_str == "1-0": score = 1.0
    elif result_str == "0-1": score = 0.0

    # 4. Save all positions from this game paired with the final outcome
    for f in game_fens:
        dataset.append(f"{f} | {score}")

    print(f"Game {game_id+1}/{NUM_GAMES} finished! Result: {result_str}. Total Data Points: {len(dataset)}")

# Shutdown the C++ engine
engine.stdin.write("quit\n")
engine.stdin.flush()

# Write the massive dataset to a text file
with open("dataset.txt", "w") as f:
    for data in dataset:
        f.write(data + "\n")

print(f"\nSUCCESS! Wrote {len(dataset)} unique chess positions to 'dataset.txt'.")
