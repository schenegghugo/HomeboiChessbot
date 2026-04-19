import torch
import torch.nn as nn
import os

# The exact same architecture
class ChessboiNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(49152, 256)
        self.fc2 = nn.Linear(512, 1)

print("Loading 12.5 Million weights from raw_weights.txt...")
print("(This takes a few seconds...)")

try:
    with open("raw_weights.txt", "r") as f:
        lines = f.readlines()
except FileNotFoundError:
    print("Error: Could not find raw_weights.txt!")
    exit(1)

# Convert all text lines to floats
weights = [float(x.strip()) for x in lines]

print(f"Loaded {len(weights)} weights. Reconstructing PyTorch Brain...")

# PyTorch network sizes
fc1_w_size = 256 * 49152
fc1_b_size = 256
fc2_w_size = 1 * 512
fc2_b_size = 1

idx = 0

# 1. fc1 Weights
fc1_w = torch.tensor(weights[idx : idx + fc1_w_size]).reshape(256, 49152)
idx += fc1_w_size

# 2. fc1 Bias
fc1_b = torch.tensor(weights[idx : idx + fc1_b_size])
idx += fc1_b_size

# 3. fc2 Weights
fc2_w = torch.tensor(weights[idx : idx + fc2_w_size]).reshape(1, 512)
idx += fc2_w_size

# 4. fc2 Bias
fc2_b = torch.tensor(weights[idx : idx + fc2_b_size])

# Inject into the model
model = ChessboiNet()
with torch.no_grad(): # Disable gradients for direct injection
    model.fc1.weight.copy_(fc1_w)
    model.fc1.bias.copy_(fc1_b)
    model.fc2.weight.copy_(fc2_w)
    model.fc2.bias.copy_(fc2_b)

# Save to the new, safe PyTorch format!
checkpoint = {
    'epoch': 0, 
    'model_state': model.state_dict(),
    # Note: We lost Adam's optimizer state since the old script didn't save it,
    # but that's okay. The new train.py will just start a fresh optimizer 
    # with your fully trained weights!
}

torch.save(checkpoint, "chessboi_checkpoint.pth")

print("\nSUCCESS! Your 7-day training run has been recovered and packaged into 'chessboi_checkpoint.pth'.")
print("You can now safely run the NEW train.py!")
