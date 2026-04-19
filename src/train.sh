#!/bin/bash
echo "Starting Daily Pipeline..."

# 1. Read the NEXT 5,000 games from the massive PGN and generate training_data.epd
python3 lichessData.py

# 2. Train the neural network on this new batch of data
python3 train.py

# 3. Move the newly trained brain into your C++ source folder
# (Assuming your python scripts are in a 'scripts' folder and C++ is in 'src')
mv chessboi.bin ../src/chessboi.bin

echo "Daily Training Complete! ChessboiV4 has digested new games."
