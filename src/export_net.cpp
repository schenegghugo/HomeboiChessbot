#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

constexpr int NUM_FEATURES = 49152; 
constexpr int HIDDEN_SIZE = 256;

// Your highly optimized, aligned NNUE format!
struct alignas(64) NNUE_Params {
    int16_t feature_weights[NUM_FEATURES * HIDDEN_SIZE];
    int16_t feature_bias[HIDDEN_SIZE];
    int16_t output_weights[HIDDEN_SIZE * 2]; // 512 total (256 White + 256 Black)
    int32_t output_bias;
};

NNUE_Params net;

int main() {
    std::cout << "Loading 12.5+ Million raw weights from PyTorch...\n";
    std::ifstream file("raw_weights.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Could not find raw_weights.txt!\n";
        return 1;
    }

    // Allocate memory matching the NEW 49,152 Python Model
    std::vector<float> fc1_w(NUM_FEATURES * HIDDEN_SIZE);
    std::vector<float> fc1_b(HIDDEN_SIZE);
    std::vector<float> fc2_w(HIDDEN_SIZE * 2);
    std::vector<float> fc2_b(1);

    // Read PyTorch Weights (Order matches exactly how Python exported them)
    std::cout << "Reading Layer 1 Weights...\n";
    for (int i = 0; i < NUM_FEATURES * HIDDEN_SIZE; i++) file >> fc1_w[i];
    
    std::cout << "Reading Layer 1 Biases...\n";
    for (int i = 0; i < HIDDEN_SIZE; i++) file >> fc1_b[i];
    
    std::cout << "Reading Layer 2 Weights...\n";
    for (int i = 0; i < HIDDEN_SIZE * 2; i++) file >> fc2_w[i];
    
    std::cout << "Reading Layer 2 Bias...\n";
    for (int i = 0; i < 1; i++) file >> fc2_b[0];

    std::cout << "Quantizing weights to AVX2 INT16 format...\n";
    // Quantization Scales (Standard Stockfish/NNUE Math)
    const float QA = 255.0f;
    const float QB = 64.0f;

    // 1. Quantize and Transpose the 49,152 Features
    // PyTorch saves them as [Out][In], but C++ needs them as [In][Out] for contiguous AVX memory!
    for (int in = 0; in < NUM_FEATURES; in++) {
        for (int out = 0; out < HIDDEN_SIZE; out++) {
            int pt_idx = out * NUM_FEATURES + in;  // PyTorch format
            int cpp_idx = in * HIDDEN_SIZE + out;  // C++ AVX2 format
            
            float val = fc1_w[pt_idx] * QA;
            net.feature_weights[cpp_idx] = std::clamp((int)std::round(val), -32768, 32767);
        }
    }

    // 2. Quantize Feature Biases
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        float val = fc1_b[i] * QA;
        net.feature_bias[i] = std::clamp((int)std::round(val), -32768, 32767);
    }

    // 3. Quantize Output Layer Weights
    for (int i = 0; i < HIDDEN_SIZE * 2; i++) {
        float val = fc2_w[i] * QB;
        net.output_weights[i] = std::clamp((int)std::round(val), -32768, 32767);
    }

    // 4. Quantize Output Bias (Int32)
    float val = fc2_b[0] * QA * QB;
    net.output_bias = (int32_t)std::round(val);

    // Write the highly optimized binary file
    std::ofstream outfile("chessboi.bin", std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(&net), sizeof(net));
    outfile.close();

    std::cout << "SUCCESS! Compiled AI Brain into 'chessboi.bin'.\n";
    std::cout << "File size: " << sizeof(net) / 1024 / 1024 << " MB.\n";
    
    return 0;
}
