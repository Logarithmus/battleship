#ifndef BATTLESHIP_UTIL_RESULT_HPP
#define BATTLESHIP_UTIL_RESULT_HPP

#include <variant>

namespace battleship {
	template<typename T, typename E>
	using Result = std::variant<T, E>;
}

#endif //BATTLESHIP_UTIL_RESULT_HPP
