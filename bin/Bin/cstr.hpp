#ifndef C_STR
#define C_STR

#include "inf.hpp"
#include <cstring>
#include <cstddef>

namespace csm {
	[[nodiscard]] inline std::size_t len(const c_string& str) noexcept {
		const char* ptr = static_cast<const char*>(str);
		return ptr ? std::strlen(ptr) : 0;
	} 
	inline bool checkstr(const c_string& src, const char* target) {
		return src == target;
	}
	inline bool checkchr(const char* src, const char* target) { return src == target; }
	c_string strip(c_string& src, const char* target) {
		if (src.length == 0 || !target || target[0] == '\0') {
			return c_string("");
		}

		size_t target_len = std::strlen(target);
		int pos = src.find(target, target_len, 0);
		if (pos == -1) {
			return c_string(""); // Target not found
		}

		// 1. Capture the removed word to return
		c_string removed(src.data + pos, target_len);

		// 2. Erase target from 'src' in-place by shifting remaining bytes left
		size_t tail_len = src.length - (pos + target_len);
		if (tail_len > 0) {
			std::memmove(src.data + pos, src.data + pos + target_len, tail_len);
		}
		src.length -= target_len;
		src.data[src.length] = '\0';

		return removed;
	}

	// Overload for c_string target
	c_string strip(c_string& src, const c_string& target) {
		return strip(src, target.data);
	}
	c_string replace(const c_string& src, const char* target, const char* replacement = "") {
		if (src.length == 0 || !target || target[0] == '\0') {
			return src;
		}

		size_t target_len = std::strlen(target);
		size_t repl_len = replacement ? std::strlen(replacement) : 0;

		int pos = src.find(target, target_len, 0);
		if (pos == -1) return src; // Target not found

		c_string result;
		size_t src_idx = 0;
		size_t res_idx = 0;

		while (pos != -1 && res_idx < c_string::MAX_SIZE - 1) {
			// Copy segment before match
			size_t segment_len = pos - src_idx;
			if (res_idx + segment_len >= c_string::MAX_SIZE - 1) break;
			std::memcpy(result.data + res_idx, src.data + src_idx, segment_len);
			res_idx += segment_len;

			// Copy replacement text
			if (repl_len > 0) {
				if (res_idx + repl_len >= c_string::MAX_SIZE - 1) break;
				std::memcpy(result.data + res_idx, replacement, repl_len);
				res_idx += repl_len;
			}

			src_idx = pos + target_len;
			pos = src.find(target, target_len, src_idx);
		}

		// Copy remaining tail
		if (src_idx < src.length && res_idx < c_string::MAX_SIZE - 1) {
			size_t tail_len = src.length - src_idx;
			if (res_idx + tail_len >= c_string::MAX_SIZE - 1) {
				tail_len = c_string::MAX_SIZE - 1 - res_idx;
			}
			std::memcpy(result.data + res_idx, src.data + src_idx, tail_len);
			res_idx += tail_len;
		}

		result.data[res_idx] = '\0';
		result.length = res_idx;
		return result;
	}
	inline c_string erasestr(c_string& src, const c_string& word) {
		return replace(src, word, "");
	}
}

#endif