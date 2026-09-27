#include "encryption.hpp"
#include <string>

std::string __PolybiusSquare__::Encrypt(std::string text) {
    std::string cipher = "";
    for (char c : text) {
        if (c == ' ') {
            cipher += " ";
            continue;
        }

        char target = c;
        if (target == 'J') target = 'I';
        if (target == 'j') target = 'i';

        bool found = false;
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 5; j++) {
                if (grid[i][j] == target) {
                    // Format row as 2 digits (00-15) and col as 1 digit (0-4) to prevent coordinate overlap
                    if (i < 10) {
                        cipher += "0";
                    }
                    cipher += std::to_string(i) + std::to_string(j);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
    return cipher;
}

std::string __PolybiusSquare__::Decrypt(std::string cipher) {
    std::string plain = "";
    for (size_t i = 0; i < cipher.length(); i++) {
        if (cipher[i] == ' ') {
            plain += " ";
            continue;
        }

        // Read 2 digits for row (00-15) and 1 digit for col (0-4)
        if (i + 2 < cipher.length()) {
            int row = (cipher[i] - '0') * 10 + (cipher[i + 1] - '0');
            int col = cipher[i + 2] - '0';

            if (row >= 0 && row < 16 && col >= 0 && col < 5) {
                plain += grid[row][col];
            }
            i += 2; // Skip the remaining digits of this 3-digit coordinate block
        }
    }
    return plain;
}