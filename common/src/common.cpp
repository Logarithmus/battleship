#include "../include/common.hpp"

namespace battleship {
	constexpr Offset Offset::DOWN  { 1,  0};
	constexpr Offset Offset::UP    {-1,  0};
	constexpr Offset Offset::RIGHT { 0,  1};
	constexpr Offset Offset::LEFT  { 0, -1};
}
