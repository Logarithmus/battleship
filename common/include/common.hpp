#ifndef BATTLESHIP_COMMON_HPP
#define BATTLESHIP_COMMON_HPP

#include <bitset>
#include <optional>
#include <variant>
#include <array>
#include <numeric>
#include <tuple>
#include <algorithm>
#include <boost/container/static_vector.hpp>
#include "util/geometry.hpp"
#include "util/num_types.hpp"
#include "util/result.hpp"

namespace battleship {
	template<u8 ROWS, u8 COLS>
	struct Grid {
		static constexpr size_t SIZE = ROWS * COLS;

		inline bool place_ship(const Position& pos) {
			std::optional<size_t> i = index(pos);
			if (i.has_value()) { ships[*i] = true; }
			return i.has_value();
		}

		[[nodiscard]] inline bool has_ship(const Position& pos) const {
			std::optional<size_t> i = index(pos);
			return i.has_value() && ships[*i];
		}

		bool place_shot(const Position& pos) {
			std::optional<size_t> i = index(pos);
			if (i.has_value()) { shots[*i] = true; }
			return i.has_value();
		}

		[[nodiscard]] inline bool has_shot(const Position& pos) const {
			std::optional<u8> i = index(pos);
			return i.has_value() && shots[*i];
		}

	private: 
		std::bitset<SIZE> ships;
		std::bitset<SIZE> shots;

		inline static std::optional<size_t> index(const Position& pos) {
			if (pos.row < ROWS && pos.col < COLS) {
				return COLS * pos.row + pos.col;
			}
			return {};
		}
	};


	struct Ship {
		Rectangle zone;

		[[nodiscard]] inline u8 length() const {
			return std::max(zone.width(), zone.height());
		}

		[[nodiscard]] inline bool touches(const Ship& other) const {
			return zone.touches_or_intersects(other.zone);
		}
	};

	template<u8 SHIP_TYPE_COUNT>
	using Rules = std::array<u8, SHIP_TYPE_COUNT>;

	constexpr Rules<4> POST_SOVIET_RULES {4, 3, 2, 1};
	constexpr Rules<5> AMERICAN_RULES {0, 1, 2, 1, 1};

	enum class ShipPlacementError {
		WrongLength,
		OutOfBounds,
		Overlap,
		TooManyShips
	};

	template<u8 ROWS, u8 COLS, Rules RULES>
	struct Ships {
		static constexpr Rectangle RECT {{0, 0}, {ROWS, COLS}};
		static constexpr size_t SHIP_COUNT {std::accumulate(RULES.begin(), RULES.end())};

		inline Result<std::monostate, ShipPlacementError> push(Ship&& ship) {
			u8 len = ship.length();
			if (len > ships.size() || len == 0) {
				return ShipPlacementError::WrongLength;
			}
			if (is_out_of_bounds(ship)) {
				return ShipPlacementError::OutOfBounds;
			}
			if (overlaps()) {
				return ShipPlacementError::Overlap;
			}
			if (is_full()) {
				return ShipPlacementError::TooManyShips;
			}

			ships.emplace_back(ship);

			return {};
		}

		[[nodiscard]] inline bool is_out_of_bounds(const Ship& ship) const {
			return !RECT.contains(ship.zone);
		}

		[[nodiscard]] inline bool overlaps(const Ship& ship) const {
			return std::any_of(ships.begin(), ships.end(),
				[&ship](const auto& other){ return ship.touches(other); }
			);
		}

		[[nodiscard]] inline bool is_full() const {
			return (ships.size() < ships.capacity())
				&& std::equal(actual_count.begin(), actual_count.end(), RULES.begin());
		}
	private:
		boost::static_vector<Ship, SHIP_COUNT> ships;
		std::array<u8, RULES.size()> actual_count {};
	};

	template<u8 ROWS, u8 COLS, Rules RULES, u8 SHIP_TYPE_COUNT>
	class PlayerField {
	public:
		inline Result<PlayerField&, ShipPlacementError> place_ship(Ship&& ship) {
			auto push_result = ships.push(ship);
			if (push_result.index() == 1) {
				return std::get<1>(push_result);
			}

			for (const auto& pos: ship.zone) {
				grid.place_ship(pos);
			}
			
			return *this;
		}

		[[nodiscard]] inline bool is_full() const {
			return ships.is_full();
		}
	private:
		Grid<ROWS, COLS> grid;
		Ships<ROWS, COLS, RULES> ships;
	};
}

#endif //BATTLESHIP_COMMON_HPP
