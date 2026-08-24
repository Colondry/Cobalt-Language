#ifndef CSYSTEM
#define CSYSTEM

#include <cstdlib>
#include <string>

namespace csm {
	int WITH_FAILURE = 1;
	int WITH_SUCCESS = 0;
	
	inline void exit(const int CODE) { std::exit(CODE); }
	inline void abort() { std::abort(); }
	inline void qexit(const int CODE) { std::quick_exit(CODE); }
	inline void command(const std::string command) { std::system(command.c_str()); }
}

#endif