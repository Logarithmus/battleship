#include "util/geometry.hpp"

namespace battleship {
	// struct Offset
	constexpr Offset Offset::DOWN  { 1,  0};
	constexpr Offset Offset::UP    {-1,  0};
	constexpr Offset Offset::RIGHT { 0,  1};
	constexpr Offset Offset::LEFT  { 0, -1};
	
	Offset operator*(i8 a, const Offset& other) {
		return {static_cast<i8>(a * other.rows), static_cast<i8>(a * other.cols)};
	}
} //namespace battleship
