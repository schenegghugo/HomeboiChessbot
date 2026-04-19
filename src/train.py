import chess
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import os
import sys

# --- CONFIG ---
DATA_FILE = "/home/hugo/Work/projects/Chess/ChessboiV4/data/training_data.epd"
WEIGHTS_OUT = "raw_weights.txt"
CHECKPOINT_FILE = "chessboi_checkpoint.pth"
BATCH_SIZE = 1024 
EPOCHS = 100

# Setup Device
device = torch.device("cuda" if torch.cuda.is_available() else "mps" if torch.backends.mps.is_available() else "cpu")
print(f"Using device: {device}")

# ---------------------------------------------------------
# 1. THE NEURAL NETWORK ARCHITECTURE
# ---------------------------------------------------------
class ChessboiNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(49152, 256)
        self.fc2 = nn.Linear(512, 1)

    def forward(self, us_features, them_features):
        us_hidden = torch.clamp(self.fc1(us_features), 0.0, 1.0)
        them_hidden = torch.clamp(self.fc1(them_features), 0.0, 1.0)
        hidden = torch.cat([us_hidden, them_hidden], dim=1)
        return self.fc2(hidden)

# ---------------------------------------------------------
# 2. FEN TO FEATURES
# ---------------------------------------------------------
def fen_to_features(fen):
    board = chess.Board(fen)
    us_tensor = torch.zeros(49152)
    them_tensor = torch.zeros(49152)
    us_color = board.turn 

    wk_sq = board.king(chess.WHITE)
    bk_sq = board.king(chess.BLACK)
    if wk_sq is None: wk_sq = 0
    if bk_sq is None: bk_sq = 0

    if us_color == chess.WHITE:
        us_king = wk_sq
        them_king = bk_sq ^ 56 
    else:
        us_king = bk_sq ^ 56
        them_king = wk_sq

    for sq in chess.SQUARES:
        piece = board.piece_at(sq)
        if piece:
            p_type = piece.piece_type - 1 
            p_color = piece.color         
            
            if us_color == chess.WHITE:
                c_us   = 0 if p_color == chess.WHITE else 1
                c_them = 0 if p_color == chess.BLACK else 1
                sq_us   = sq
                sq_them = sq ^ 56 
            else:
                c_us   = 0 if p_color == chess.BLACK else 1
                c_them = 0 if p_color == chess.WHITE else 1
                sq_us   = sq ^ 56
                sq_them = sq
                
            idx_us_base   = c_us * 384 + p_type * 64 + sq_us
            idx_them_base = c_them * 384 + p_type * 64 + sq_them
            
            idx_us   = us_king * 768 + idx_us_base
            idx_them = them_king * 768 + idx_them_base
            
            us_tensor[idx_us] = 1.0
            them_tensor[idx_them] = 1.0
            
    return us_tensor, them_tensor

# ---------------------------------------------------------
# 3. CUSTOM DATASET
# ---------------------------------------------------------
class ChessDataset(Dataset):
    def __init__(self, epd_file):
        self.fens = []
        self.targets = []
        
        print(f"Loading {epd_file} into memory (FENs only)...")
        with open(epd_file, "r") as f:
            for line in f:
                if ' c9 "' not in line: 
                    continue
                parts = line.split(' c9 "')
                fen = parts[0].strip()
                score_str = parts[1].replace('";', '').strip()
                try:
                    white_score = float(score_str)
                except ValueError:
                    continue 
                board = chess.Board(fen.split()[0] + " " + fen.split()[1])
                stm_score = white_score if board.turn == chess.WHITE else -white_score
                
                self.fens.append(fen)
                self.targets.append([stm_score])
                
        print(f"Successfully loaded {len(self.fens)} positions into memory!")

    def __len__(self):
        return len(self.fens)

    def __getitem__(self, idx):
        fen = self.fens[idx]
        target = torch.tensor(self.targets[idx], dtype=torch.float32)
        us_t, them_t = fen_to_features(fen)
        return us_t, them_t, target

# ---------------------------------------------------------
# 4. SAVE & EXPORT HELPER FUNCTION
# ---------------------------------------------------------
def save_progress(model, optimizer, epoch):
    print(f"\n[SAVE] Saving brain to {CHECKPOINT_FILE}...")
    checkpoint = {
        'epoch': epoch,
        'model_state': model.state_dict(),
        'optimizer_state': optimizer.state_dict()
    }
    torch.save(checkpoint, CHECKPOINT_FILE)
    
    print(f"[SAVE] Exporting C++ weights to {WEIGHTS_OUT}...")
    with open(WEIGHTS_OUT, "w") as f:
        for val in model.fc1.weight.detach().cpu().flatten().tolist(): f.write(f"{val}\n")
        for val in model.fc1.bias.detach().cpu().flatten().tolist(): f.write(f"{val}\n")
        for val in model.fc2.weight.detach().cpu().flatten().tolist(): f.write(f"{val}\n")
        for val in model.fc2.bias.detach().cpu().flatten().tolist(): f.write(f"{val}\n")
    print("[SAVE] Done! Safe to exit if needed.\n")

# ---------------------------------------------------------
# 5. THE TRAINING LOOP
# ---------------------------------------------------------
if __name__ == "__main__":
    if not os.path.exists(DATA_FILE):
        print(f"ERROR: Could not find {DATA_FILE}")
        sys.exit(1)

    dataset = ChessDataset(DATA_FILE)
    dataloader = DataLoader(dataset, batch_size=BATCH_SIZE, shuffle=True, num_workers=4, pin_memory=True)
    
    print("Initializing Chessboi Neural Network...")
    model = ChessboiNet().to(device)
    optimizer = optim.Adam(model.parameters(), lr=0.0001) 
    criterion = nn.MSELoss() 
    
    start_epoch = 1
    
    # ---------------------------------------------------------
    # SMART LOAD
    # ---------------------------------------------------------
    if os.path.exists(CHECKPOINT_FILE):
        print(f"Found existing brain! Loading from {CHECKPOINT_FILE}...")
        checkpoint = torch.load(CHECKPOINT_FILE, map_location=device)
        
        # Check if it's our new smart checkpoint format, or the old legacy format
        if 'model_state' in checkpoint:
            model.load_state_dict(checkpoint['model_state'])
            optimizer.load_state_dict(checkpoint['optimizer_state'])
            start_epoch = checkpoint['epoch'] + 1
            print(f"Resuming automatically from Epoch {start_epoch}!")
        else:
            model.load_state_dict(checkpoint) # Legacy fallback
            print("Loaded legacy weights. Starting at Epoch 1 (but keeping brain intact!)")

    print("\nStarting Training... (Press Ctrl+C at any time to pause and save)")
    
    current_epoch = start_epoch
    
    try:
        for epoch in range(start_epoch, EPOCHS + 1):
            current_epoch = epoch
            total_loss = 0.0
            batches_processed = 0
            
            for batch_us, batch_them, batch_targets in dataloader:
                batch_us = batch_us.to(device)
                batch_them = batch_them.to(device)
                batch_targets = batch_targets.to(device)
                
                optimizer.zero_grad()
                predictions = model(batch_us, batch_them)
                loss = criterion(predictions, batch_targets)
                loss.backward()
                optimizer.step()
                
                total_loss += loss.item() * batch_us.size(0)
                batches_processed += 1
                
                if batches_processed % 500 == 0:
                    print(f"  ... Batch {batches_processed} / {len(dataloader)}")
                
            avg_loss = total_loss / len(dataset)
            print(f"Epoch {epoch}/{EPOCHS} | Network Error (MSE Loss): {avg_loss:.4f}")
            
            # Save safely at the end of every single epoch
            save_progress(model, optimizer, epoch)
                
        print("\nAll 100 Epochs Complete!")

    # ---------------------------------------------------------
    # CTRL+C SAFETY NET
    # ---------------------------------------------------------
    except KeyboardInterrupt:
        print("\n\n[!] CTRL+C DETECTED!")
        print("[!] Emergency Save Triggered. Please wait a moment...")
        save_progress(model, optimizer, current_epoch)
        print("[!] Safe to close now. Run the script again later to resume.")
        sys.exit(0)
