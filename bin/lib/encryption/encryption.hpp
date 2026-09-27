#ifndef ENCRYPTION
#define ENCRYPTION

#include <string>

class __StreamCipher__ {
public:
	std::string EncryptStr(std::string word);
	std::string DecryptStr(std::string word);
};
class __PolybiusSquare__ {
public:
    std::string Encrypt(std::string text);
    std::string Decrypt(std::string text);
private:
    char grid[16][5] = {
    {'A', 'B', 'C', 'D', 'E'},
    {'F', 'G', 'H', 'I', 'K'},
    {'L', 'M', 'N', 'O', 'P'},
    {'Q', 'R', 'S', 'T', 'U'},
    {'V', 'W', 'X', 'Y', 'Z'},
    {'a', 'b', 'c', 'd', 'e'},
    {'f', 'g', 'h', 'i', 'k'},
    {'l', 'm', 'n', 'o', 'p'},
    {'q', 'r', 's', 't', 'u'},
    {'v', 'w', 'x', 'y', 'z'},
    {'!', '@', '#', '$', '%'},
    {'^', '&', '*', '(', ')'},
    {'`', '=', '+', '_', '-'},
    {'\\', ']', '[', '{', '}'},
    {':', ';', '\'', '"', '/'},
    {',', '.', '<', '>', '?'}
    };
};

#endif