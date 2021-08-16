#ifndef BATTLESHIP_UTIL_RESULT_HPP
#define BATTLESHIP_UTIL_RESULT_HPP

#include <variant>

namespace battleship {
	template<typename T, typename E>
	using Result = std::variant<T, E>;

	template<typename T, typename E>
	[[nodiscard]] inline bool is_ok(const Result<T, E>& result) {
		return result.index() == 0;
	}

	template<typename T, typename E>
	[[nodiscard]] inline bool is_err(const Result<T, E>& result) {
		return result.index() == 1;
	}
}

#endif //BATTLESHIP_UTIL_RESULT_HPP
