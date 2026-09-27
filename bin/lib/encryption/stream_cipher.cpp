#include <string>
#include <cstring>
#include "encryption.hpp"

std::string __StreamCipher__::EncryptStr(std::string word) {
	size_t wlen = std::strlen(word.c_str());
	char cword[256];
	size_t length = word.copy(cword, 255);
	cword[length] = '\0';

	for (int i = 0; i < wlen; i++) {
		cword[i] = cword[i] ^ wlen;
	}
	return cword;
}
std::string __StreamCipher__::DecryptStr(std::string word) {
	size_t wlen = std::strlen(word.c_str());
	char cword[256];
	size_t length = word.copy(cword, 255);
	cword[length] = '\0';

	for (int i = 0; i < wlen; i++) {
		cword[i] = cword[i] ^ wlen;
	}
	return cword;
}