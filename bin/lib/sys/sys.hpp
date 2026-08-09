#ifndef SYS
#define SYS

#include <string>

class __System__ {
public:
	void Clear();
	bool isNative(std::string pf);
	void clearLines(int lines);

	void* Malloc(size_t size);
	void* ReAlloc(void* ptr, size_t size);
	void Free(void* ptr);
};

#endif