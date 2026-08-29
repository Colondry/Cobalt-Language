#ifndef C_STR
#define C_STR

#include "inf.hpp"
#include <cstring>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace csm {
	// Templated strlen to automatically unwrap unique_ptr<c_string> and c_string*
	template <typename T>
	[[nodiscard]] inline std::size_t strlen(T&& str) noexcept {
		decltype(auto) u_str = unwrap_val(std::forward<T>(str));
		const char* ptr = static_cast<const char*>(u_str);
		return ptr ? std::strlen(ptr) : 0;
	} 

	template <typename T1, typename T2>
	inline bool checkstr(T1&& src, T2&& target) {
		return unwrap_val(std::forward<T1>(src)) == unwrap_val(std::forward<T2>(target));
	}

	template <typename T1, typename T2>
	inline bool checkchr(T1&& src, T2&& target) { 
		return unwrap_val(std::forward<T1>(src)) == unwrap_val(std::forward<T2>(target)); 
	}

	// Internal implementation for in-place string stripping
	inline c_string strip_impl(c_string& src, const char* target) {
		if (src.length == 0 || !target || target[0] == '\0') {
			return c_string("");
		}

		size_t target_len = std::strlen(target);
		int pos = src.find(target, target_len, 0);
		if (pos == -1) {
			return c_string("");
		}

		c_string removed(src.data + pos, target_len);

		size_t tail_len = src.length - (pos + target_len);
		if (tail_len > 0) {
			std::memmove(src.data + pos, src.data + pos + target_len, tail_len);
		}
		src.length -= target_len;
		src.data[src.length] = '\0';

		return removed;
	}

	template <typename T1, typename T2>
	inline c_string strip(T1&& src, T2&& target) {
		decltype(auto) u_src = unwrap_val(std::forward<T1>(src));
		decltype(auto) u_target = unwrap_val(std::forward<T2>(target));
		
		if constexpr (std::is_convertible_v<decltype(u_target), const char*>) {
			return strip_impl(u_src, static_cast<const char*>(u_target));
		} else {
			return strip_impl(u_src, u_target.data);
		}
	}

	// Internal implementation for string replacement
	inline c_string replace_impl(const c_string& src, const char* target, const char* replacement = "") {
		if (src.length == 0 || !target || target[0] == '\0') {
			return src;
		}

		size_t target_len = std::strlen(target);
		size_t repl_len = replacement ? std::strlen(replacement) : 0;

		int pos = src.find(target, target_len, 0);
		if (pos == -1) return src;

		c_string result;
		size_t src_idx = 0;
		size_t res_idx = 0;

		while (pos != -1 && res_idx < c_string::MAX_SIZE - 1) {
			size_t segment_len = pos - src_idx;
			if (res_idx + segment_len >= c_string::MAX_SIZE - 1) break;
			std::memcpy(result.data + res_idx, src.data + src_idx, segment_len);
			res_idx += segment_len;

			if (repl_len > 0) {
				if (res_idx + repl_len >= c_string::MAX_SIZE - 1) break;
				std::memcpy(result.data + res_idx, replacement, repl_len);
				res_idx += repl_len;
			}

			src_idx = pos + target_len;
			pos = src.find(target, target_len, src_idx);
		}

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

	template <typename T1, typename T2, typename T3 = const char*>
	inline c_string replace(T1&& src, T2&& target, T3&& replacement = "") {
		decltype(auto) u_src = unwrap_val(std::forward<T1>(src));
		decltype(auto) u_target = unwrap_val(std::forward<T2>(target));
		decltype(auto) u_repl = unwrap_val(std::forward<T3>(replacement));

		const char* target_ptr = static_cast<const char*>(u_target);
		const char* repl_ptr = static_cast<const char*>(u_repl);

		return replace_impl(u_src, target_ptr, repl_ptr);
	}

	template <typename T1, typename T2>
	inline c_string erasestr(T1&& src, T2&& word) {
		return replace(std::forward<T1>(src), std::forward<T2>(word), "");
	}
}

#endif