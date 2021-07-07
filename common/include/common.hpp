#ifndef BATTLESHIP_COMMON_HPP
#define BATTLESHIP_COMMON_HPP

#include <bitset>
#include <optional>
#include <variant>
#include <array>
#include <algorithm>
#include <iterator>
#include <ranges>
#include <compare>
#include "num_types.hpp"

namespace battleship {
	struct Offset {
		i8 rows;
		i8 cols;

		constexpr Offset(i8 rows, i8 cols): rows{rows}, cols{cols} {}

		inline Offset operator*(i8 other) const {
			return Offset(other * rows, other * cols);
		}

		static const Offset DOWN, UP, RIGHT, LEFT;
	};

	friend inline Offset operator*(i8 a, Position& other) {
		return Offset(a * other.rows, a * other.cols);
	}

	struct Position {
		u8 row;
		u8 col;

		constexpr Position(u8 row, u8 col): row{row}, col{col} {}

		inline Position operator+(const Offset& offset) const {
			return Position(row + offset.rows, col + offset.cols);
		}

		inline Position& operator+=(const Offset& offset) {
			row += offset.rows;
			col += offset.cols;
			return *this;
		}

		inline Position operator-(const Offset& offset) const {
			return Position(row + offset.rows, col + offset.cols);
		}

		inline Position& operator-=(const Offset& offset) {
			row -= offset.rows;
			col -= offset.cols;
			return *this;
		}

		inline auto operator<=>(const Position&) const = default;
	};

	// struct Interval2D {
	// 	Position first, last;

	// 	Interval2D(const Position& start, const Position& end): start{start}, end{end} {
	// 		assert(start <= end);
	// 	}

	// 	inline bool intersects(const Interval2D& other) {
	// 		return (other.start <= end) && (other.end >= start);
	// 	}

	// 	inline bool contains(const Interval2D& other) {
	// 		return (other.start >= start) && (other.end <= end);
	// 	}

	// 	class Iterator {
	// 	public:
	// 		Iterator(Position pos): pos{pos} {};

	// 		inline Position operator*() {
	// 			return pos;
	// 		}

	// 		inline Iterator& operator++() {
	// 			if (pos.row < last.row) {
	// 				
	// 			}
	// 		}

	// 		inline Iterator operator++(int) {
	// 			Iterator old = *this;
	// 			++(*this);
	// 			return old;
	// 		}
	// 		
	// 	private:
	// 		Position pos;
	// 	};

	// 	inline Iterator begin() {
	// 		return Iterator(first);
	// 	}

	// 	inline Iterator end() {
	// 		return Iterator(last);
	// 	}
	// };

	struct Ship {
		Position pos;
		u8 length;
		Offset direction;
	
		constexpr Ship(Position pos, u8 length, Offset direction):
			pos{pos}, length{length}, direction{direction} {}

		Position end() const {
			return pos + length * direction;
		}
	};

	template<u8 ROWS, u8 COLS>
	class Grid {
	public:
		static constexpr size_t SIZE = ROWS * COLS;

		bool place_ship(const Position& pos) {
			std::optional<size_t> i = index(pos);
			if (i) { ships[*i]; }
			return i.has_value();
		}

		bool has_ship(const Position& pos) const {
			std::optional<size_t> i = index(pos);
			return i.has_value() && ships[*i];
		}

		bool place_shot(const Position& pos) {
			std::optional<size_t> i = index(pos);
			if (i) { shots[*i]; }
			return shots[i] = true;
		}

		bool has_shot(const Position& pos) const {
			std::optional<u8> i = index(pos);
			return i.has_value() && shots[*i];
		}

	private: 
		std::bitset<SIZE> ships;
		std::bitset<SIZE> shots;

		static std::optional<size_t> index(Position pos) {
			if (pos.row < ROWS && pos.col < COLS) {
				return COLS * pos.row + pos.col;
			}
			return {};
		}
	};


	template<size_t SHIP_TYPES_COUNT>
	using ShipCount = std::array<u8, SHIP_TYPES_COUNT>;

	constexpr ShipCount<4> POST_SOVIET_RULES {4, 3, 2, 1};
	constexpr ShipCount<5> AMERICAN_RULES {0, 1, 2, 1, 1};

	template<u8 ROWS, u8 COLS>
	class PlayerGrid {
	public:
		PlayerGrid() = delete;

		static void from_cbor() {
		}

	private:
		Grid<ROWS, COLS> grid;
	};

	template<typename T, typename E>
	using Result = std::variant<T, E>;

	template<u8 ROWS, u8 COLS, u8 SHIP_TYPES_COUNT>
	class PlayerGridBuilder {
	public:
		enum class ShipPlacementError {
			WrongLength,
			Overlap,
			OutOfBounds,
			MissingShips,
			TooManyShips
		};

		Result<PlayerGridBuilder&, ShipPlacementError> place_ship(
				const Ship& ship,
				const Offset& direction,
				Position pos) {
			if (ship.length > required_ships.size()) {
				return ShipPlacementError::WrongLength;
			} else if (is_out_of_bounds(ship)) {
				return ShipPlacementError::OutOfBounds;
			} else if (overlaps(ship)) {
				return ShipPlacementError::Overlap;
			}

			for (int i = 0; i < ship.length; ++i) {
				pos += direction;
				player_grid.place_ship(pos);
			}
			return *this;
		}

		std::optional<PlayerGrid<ROWS, COLS>> build() {
			player_grid
		}
	private:
		inline bool is_out_of_bounds(const Ship& ship) const {
			return !is_in_bounds(ship);
		}

		inline bool is_in_bounds(const Ship& ship) const {
			return (ship.pos >= {0, 0}) && (ship.pos.end() <= {ROWS, COLS});
		}

		inline bool overlaps(const Ship& ship) const {
			const auto ship_end = ship.end();
			for (int row = ship.pos.row - 1; row <= ship_end.row; ++row) {
				for (int col = ship.pos.col - 1; col <= ship_end.col; ++col) {
					auto has_ship = player_grid.has_ship({row, col});
					if (has_ship && *has_ship) {
						return true;
					}
				}
			}
			return false;
		}

		PlayerGrid<ROWS, COLS> player_grid;
		ShipCount<SHIP_TYPES_COUNT> required_ships;
	};
}

#endif //BATTLESHIP_COMMON_HPP
