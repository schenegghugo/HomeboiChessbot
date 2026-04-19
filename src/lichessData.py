import chess
import chess.pgn
import chess.engine
import os
import glob
import time

# --- CONFIG ---
DATA_DIR = "/home/hugo/Work/projects/Chess/ChessboiV4/data"
PGN_DIR = f"{DATA_DIR}/Lichess Elite Database"
OUTPUT_EPD = f"{DATA_DIR}/training_data.epd"

STOCKFISH_PATH = "/home/hugo/Work/projects/Chess/Stockfish/stockfish/src/Stockfish-sf_18/src/stockfish" 
DEPTH = 10 
THREADS = 4 # Adjust to your Thinkpad's core count
EVAL_EVERY_N_MOVES = 3 # Evaluates 1 position every 3 plies to prevent dataset bias & speed things up
MAX_POSITIONS = 5_000_000 # Stop after generating 5 million positions (plenty for V4)

def generate_data():
    if not os.path.exists(STOCKFISH_PATH):
        print(f"Error: Could not find Stockfish at {STOCKFISH_PATH}")
        return

    # Find all PGN files and sort them chronologically
    pgn_files = sorted(glob.glob(os.path.join(PGN_DIR, "*.pgn")))
    if not pgn_files:
        print(f"Error: No PGN files found in {PGN_DIR}")
        return

    print(f"Found {len(pgn_files)} PGN files. Starting Stockfish 18...")
    engine = chess.engine.SimpleEngine.popen_uci(STOCKFISH_PATH)
    engine.configure({"Threads": THREADS, "Hash": 256})
    
    total_games_processed = 0
    total_positions_saved = 0
    start_time = time.time()

    try:
        # Open in 'write' mode initially to start fresh. 
        # (Change to "a" if you want to stop and resume later without overwriting)
        with open(OUTPUT_EPD, "w") as epd_file:
            
            for pgn_path in pgn_files:
                filename = os.path.basename(pgn_path)
                print(f"\n--- Now processing {filename} ---")
                
                with open(pgn_path, "r") as pgn_file:
                    while total_positions_saved < MAX_POSITIONS:
                        game = chess.pgn.read_game(pgn_file)
                        if game is None:
                            break # End of this PGN file, move to the next one
                            
                        board = game.board()
                        ply_count = 0
                        
                        for move in game.mainline_moves():
                            board.push(move)
                            ply_count += 1
                            
                            # Skip opening book (first 12 plies) and only take every Nth move
                            if ply_count < 12 or ply_count % EVAL_EVERY_N_MOVES != 0:
                                continue
                                
                            # Analyze position
                            info = engine.analyse(board, chess.engine.Limit(depth=DEPTH))
                            score = info["score"].white()
                            
                            if score.is_mate():
                                eval_val = 32000 if score.mate() > 0 else -32000
                            else:
                                eval_val = score.score()

                            # Save to EPD
                            epd_line = f"{board.fen()} c9 \"{eval_val}\";\n"
                            epd_file.write(epd_line)
                            total_positions_saved += 1
                            
                        total_games_processed += 1
                        
                        # Logging
                        if total_games_processed % 100 == 0:
                            elapsed = time.time() - start_time
                            pos_per_sec = total_positions_saved / elapsed if elapsed > 0 else 0
                            print(f"[{filename}] Games: {total_games_processed} | Positions: {total_positions_saved} | Speed: {pos_per_sec:.1f} pos/sec")

                if total_positions_saved >= MAX_POSITIONS:
                    print(f"\nReached target of {MAX_POSITIONS} positions!")
                    break

    except KeyboardInterrupt:
        print("\n\nProcess stopped manually by user (Ctrl+C). Saving data...")
        
    finally:
        engine.quit()
        print(f"\n--- SUMMARY ---")
        print(f"Total Games Parsed: {total_games_processed}")
        print(f"Total Positions Evaluated: {total_positions_saved}")
        print(f"Data successfully saved to {OUTPUT_EPD}")

if __name__ == "__main__":
    generate_data()
